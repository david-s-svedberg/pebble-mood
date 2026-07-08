package se.svep.mood.companion.ai

import android.content.Context
import android.util.Log
import org.json.JSONArray
import org.json.JSONObject
import se.svep.mood.companion.db.AppDatabase
import se.svep.mood.companion.db.MetricEntity
import se.svep.mood.companion.db.RegistrationEntity
import se.svep.mood.companion.graph.DailyAggregation
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.OutputStream
import java.net.HttpURLConnection
import java.net.URL
import java.time.Instant
import java.time.LocalDate
import java.time.ZoneId
import javax.net.ssl.HttpsURLConnection

/**
 * The AI half of phase 7: dumps a period's per-day metric values (mood +
 * Health Connect) to the Claude API and returns its reasoning about what seems
 * to influence what. This is the ONLY path that sends data off the phone, and
 * only on an explicit tap after consent.
 */
object AiAnalyzer {

    private const val TAG = "MoodAI"
    private const val ENDPOINT = "https://api.anthropic.com/v1/messages"

    sealed interface Result {
        data class Ok(val text: String) : Result
        data class Error(val message: String) : Result
    }

    suspend fun analyze(context: Context, days: Int): Result {
        val key = AiSettings.apiKey(context)
        if (key.isBlank()) return Result.Error("Ingen API-nyckel angiven.")

        val db = AppDatabase.get(context)
        val metrics = db.metrics().all()
        val from = System.currentTimeMillis() / 1000 - days * 86_400L
        val regs = db.registrations().inWindow(metrics.map { it.metricId }, from)
        if (regs.isEmpty()) return Result.Error("Ingen data i perioden.")

        val payload = buildPayload(metrics, regs, days)
        return try {
            callClaude(key, payload)
        } catch (e: Exception) {
            Log.e(TAG, "analysis failed", e)
            Result.Error("Nätverksfel: ${e.message}")
        }
    }

    /** Compact JSON: metric metadata + a per-day table of daily values. */
    private fun buildPayload(
        metrics: List<MetricEntity>,
        registrations: List<RegistrationEntity>,
        days: Int,
        zone: ZoneId = ZoneId.systemDefault(),
    ): String {
        val series: Map<Int, Map<LocalDate, Double>> = metrics.associate { m ->
            m.metricId to registrations
                .filter { it.metricId == m.metricId }
                .groupBy { Instant.ofEpochSecond(it.timestamp).atZone(zone).toLocalDate() }
                .mapValues { (_, dayRegs) ->
                    if (m.type == "bool") DailyAggregation.boolDayValue(dayRegs).toDouble()
                    else DailyAggregation.dailyValue(dayRegs).toDouble()
                }
        }
        val present = metrics.filter { (series[it.metricId]?.isNotEmpty()) == true }

        val metricsArr = JSONArray()
        present.forEach { m ->
            metricsArr.put(
                JSONObject()
                    .put("name", m.name)
                    .put("type", m.type)
                    .put("range", "${m.min}-${m.max}")
                    .put("valence", valenceWord(m.valence))
            )
        }

        val allDays = present.flatMap { series[it.metricId]!!.keys }.toSortedSet()
        val daily = JSONArray()
        allDays.forEach { day ->
            val row = JSONObject().put("date", day.toString())
            present.forEach { m ->
                series[m.metricId]!![day]?.let { v ->
                    row.put(m.name, Math.round(v * 10) / 10.0)   // 1 decimal
                }
            }
            daily.put(row)
        }

        return JSONObject()
            .put("period_days", days)
            .put("metrics", metricsArr)
            .put("daily", daily)
            .toString()
    }

    private fun valenceWord(v: Int) = when {
        v > 0 -> "positive"   // high value is good
        v < 0 -> "negative"   // high value is bad
        else -> "neutral"
    }

    private fun callClaude(apiKey: String, payload: String): Result {
        val system =
            "Du är en analytiker för en humör-dagbok. Användaren skickar dagliga värden för " +
                "olika metrics (humör, ångest, sömn, steg, puls m.m.) med varje metrics valens " +
                "(positive = högt värde är bra, negative = högt värde är dåligt). Leta efter " +
                "mönster och möjliga samband — vad verkar påverka vad, inklusive effekter dagen " +
                "efter. Svara på svenska, kortfattat och konkret (punktlista). Var tydlig med att " +
                "det är samband och inte bevisad orsak, och nämn om underlaget är tunt."

        val body = JSONObject()
            .put("model", AiSettings.MODEL)
            .put("max_tokens", 1024)
            .put("system", system)
            .put(
                "messages",
                JSONArray().put(
                    JSONObject()
                        .put("role", "user")
                        .put("content", "Analysera min data och lyft de tydligaste mönstren:\n$payload")
                )
            )
            .toString()

        val conn = URL(ENDPOINT).openConnection() as HttpsURLConnection
        conn.requestMethod = "POST"
        conn.connectTimeout = 15_000
        conn.readTimeout = 60_000
        conn.doOutput = true
        conn.setRequestProperty("content-type", "application/json")
        conn.setRequestProperty("x-api-key", apiKey)
        conn.setRequestProperty("anthropic-version", "2023-06-01")

        conn.outputStream.use { os: OutputStream -> os.write(body.toByteArray(Charsets.UTF_8)) }

        val status = conn.responseCode
        val stream = if (status in 200..299) conn.inputStream else conn.errorStream
        val text = BufferedReader(InputStreamReader(stream, Charsets.UTF_8)).use { it.readText() }
        conn.disconnect()

        if (status !in 200..299) {
            val msg = runCatching {
                JSONObject(text).getJSONObject("error").getString("message")
            }.getOrDefault("HTTP $status")
            return Result.Error("API-fel: $msg")
        }

        // content is an array of blocks; concatenate the text blocks.
        val content = JSONObject(text).getJSONArray("content")
        val sb = StringBuilder()
        for (i in 0 until content.length()) {
            val block = content.getJSONObject(i)
            if (block.optString("type") == "text") sb.append(block.optString("text"))
        }
        val result = sb.toString().ifBlank { "Tomt svar." }
        return Result.Ok(result)
    }
}
