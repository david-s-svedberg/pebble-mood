package se.svep.mood.companion

import android.content.Context
import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import org.json.JSONArray
import org.json.JSONObject
import se.svep.mood.companion.db.AppDatabase
import se.svep.mood.companion.db.GroupEntity
import se.svep.mood.companion.db.MembershipEntity
import se.svep.mood.companion.db.MetricEntity
import se.svep.mood.companion.db.RegistrationEntity

/** Result of one import batch, echoed back to pkjs as the ack. */
data class ImportResult(
    val received: Int,
    val new: Int,
    /** Newest registration timestamp in the batch (unix s) — the watch may prune up to here. */
    val ackedThrough: Long,
)

/**
 * Parses a pkjs export payload (config + registrations) and stores it
 * idempotently. Emits on [dataVersion] so any UI can refresh.
 */
class ImportRepository(context: Context) {

    private val db = AppDatabase.get(context)

    suspend fun import(body: String): ImportResult {
        val payload = JSONObject(body)
        val now = System.currentTimeMillis()

        payload.optJSONObject("config")?.let { config ->
            importMetrics(config.optJSONArray("metrics") ?: JSONArray(), now)
            importGroups(config.optJSONArray("groups") ?: JSONArray())
        }

        val result = importRegistrations(payload.getJSONArray("registrations"), now)
        Log.i(TAG, "import: received=${result.received} new=${result.new} ackedThrough=${result.ackedThrough}")
        lastImportFlow.value = result to now
        bumpDataVersion()
        return result
    }

    /** Config mirrors the watch — except valence, which is ours and survives. */
    private suspend fun importMetrics(metrics: JSONArray, now: Long) {
        val rows = ArrayList<MetricEntity>()
        for (i in 0 until metrics.length()) {
            val m = metrics.getJSONObject(i)
            val id = m.getInt("metricId")
            val existingValence = db.metrics().byId(id)?.valence ?: 0
            rows.add(
                MetricEntity(
                    metricId = id,
                    name = m.optString("name", ""),
                    type = m.optString("type", "unknown"),
                    min = m.optInt("min", 0),
                    max = m.optInt("max", 0),
                    mainIcon = m.optInt("mainIcon", 0),
                    optionIcons = m.optJSONArray("optionIcons").toCsv(),
                    optionTexts = m.optJSONArray("optionTexts")?.let { arr ->
                        (0 until arr.length()).joinToString("\u001f") { arr.optString(it) }
                    } ?: "",
                    valence = existingValence,
                    lastSeenAt = now / 1000,
                )
            )
        }
        if (rows.isNotEmpty()) db.metrics().upsertAll(rows)
    }

    private suspend fun importGroups(groups: JSONArray) {
        val groupRows = ArrayList<GroupEntity>()
        for (i in 0 until groups.length()) {
            val g = groups.getJSONObject(i)
            val groupId = g.getInt("groupId")
            groupRows.add(
                GroupEntity(
                    groupId = groupId,
                    name = g.optString("name", ""),
                    hour = g.optInt("hour", 0),
                    minute = g.optInt("minute", 0),
                    active = g.optBoolean("active", true),
                )
            )
            db.groups().clearMembers(groupId)
            val members = g.optJSONArray("members") ?: JSONArray()
            db.groups().addMembers(
                (0 until members.length()).map { MembershipEntity(groupId, members.getInt(it)) }
            )
        }
        if (groupRows.isNotEmpty()) db.groups().upsertAll(groupRows)
    }

    private suspend fun importRegistrations(regs: JSONArray, now: Long): ImportResult {
        val rows = ArrayList<RegistrationEntity>(regs.length())
        var newest = 0L
        for (i in 0 until regs.length()) {
            val r = regs.getJSONObject(i)
            val timestamp = r.getLong("timestamp")
            if (timestamp > newest) newest = timestamp
            rows.add(
                RegistrationEntity(
                    metricId = r.getInt("metricId"),
                    groupId = r.optInt("groupId", 0),
                    groupName = r.optString("groupName", ""),
                    value = r.getInt("value"),
                    timestamp = timestamp,
                    importedAt = now,
                )
            )
        }
        val inserted = if (rows.isEmpty()) 0 else db.registrations().insertAll(rows).count { it != -1L }
        return ImportResult(received = rows.size, new = inserted, ackedThrough = newest)
    }

    private fun JSONArray?.toCsv(): String =
        if (this == null) "" else (0 until length()).joinToString(",") { optInt(it).toString() }

    companion object {
        private const val TAG = "MoodCompanion"

        private val lastImportFlow = MutableStateFlow<Pair<ImportResult, Long>?>(null)
        /** Latest completed import (result, wall-clock millis) — for the status text. */
        val lastImport = lastImportFlow.asStateFlow()

        private val dataVersionFlow = MutableStateFlow(0)
        /** Bumps on ANY data change (imports, demo data) — the UI refresh signal. */
        val dataVersion = dataVersionFlow.asStateFlow()

        fun bumpDataVersion() {
            dataVersionFlow.value++
        }
    }
}
