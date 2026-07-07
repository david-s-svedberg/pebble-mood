package se.svep.mood.companion

import android.os.Bundle
import android.util.Log
import android.widget.ScrollView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import org.json.JSONObject
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * Spike UI: starts the import listener and shows what arrives, raw.
 *
 * Purpose (step 1 in design/companion_app_plan.md): prove that the Pebble
 * app's JS runtime can deliver the export to us over localhost on a real
 * phone. Keep this app open, then open the Pebble app so the watchapp's JS
 * side relaunches and pushes the export. Everything else (persistence, UI,
 * correlations) comes after this is proven.
 *
 * The server is activity-scoped on purpose — a foreground service is real-app
 * territory, not spike territory.
 */
class MainActivity : AppCompatActivity() {

    private lateinit var log: TextView
    private var server: ImportServer? = null
    private var imports = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        log = TextView(this).apply {
            setPadding(32, 32, 32, 32)
            textSize = 14f
        }
        setContentView(ScrollView(this).apply { addView(log) })
        append("Mood companion — import spike\nLyssnar på 127.0.0.1:9099/import …\n")
        append("Öppna Pebble-appen så att klockans JS-sida startar om och pushar exporten.\n")
    }

    override fun onStart() {
        super.onStart()
        server = ImportServer(onImport = ::onImport).also { it.start() }
    }

    override fun onStop() {
        server?.stop()
        server = null
        super.onStop()
    }

    private fun onImport(body: String) {
        Log.i("MoodCompanion", "import: ${body.take(500)}")
        val summary = try {
            val payload = JSONObject(body)
            val regs = payload.getJSONArray("registrations")
            val first = if (regs.length() > 0) regs.getJSONObject(0).toString(2) else "(tom)"
            "✅ Import #${++imports}: ${regs.length()} registreringar\nFörsta posten:\n$first"
        } catch (e: Exception) {
            "⚠️ Kunde inte tolka payload (${e.message}):\n${body.take(300)}"
        }
        runOnUiThread {
            append("\n[${timestamp()}] $summary\n")
        }
    }

    private fun append(text: String) {
        log.append(text)
    }

    private fun timestamp(): String =
        SimpleDateFormat("HH:mm:ss", Locale.ROOT).format(Date())
}
