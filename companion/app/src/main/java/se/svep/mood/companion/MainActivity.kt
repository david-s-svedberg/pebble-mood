package se.svep.mood.companion

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.widget.ScrollView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import se.svep.mood.companion.db.AppDatabase
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * Phase-2 status view: starts the import service and shows what the database
 * holds. The real UI (history, graphs) is phase 3.
 */
class MainActivity : AppCompatActivity() {

    private lateinit var status: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        status = TextView(this).apply {
            setPadding(32, 32, 32, 32)
            textSize = 15f
        }
        setContentView(ScrollView(this).apply { addView(status) })

        requestNotificationPermissionIfNeeded()
        ImportService.start(this)

        // Refresh on every completed import (and once at startup).
        lifecycleScope.launch {
            ImportRepository.lastImport.collectLatest { refresh(it) }
        }
    }

    override fun onResume() {
        super.onResume()
        lifecycleScope.launch { refresh(ImportRepository.lastImport.value) }
    }

    private suspend fun refresh(last: Pair<ImportResult, Long>?) {
        val db = AppDatabase.get(this)
        val total = db.registrations().count()
        val newest = db.registrations().newestTimestamp()
        val perMetric = db.registrations().countsPerMetric()

        val text = buildString {
            appendLine("Mood Companion")
            appendLine("Synk-tjänsten lyssnar på localhost:9099.")
            appendLine()
            if (last != null) {
                val (result, at) = last
                appendLine("Senaste import ${time(at)}: ${result.received} mottagna, ${result.new} nya.")
            } else {
                appendLine("Ingen import den här sessionen ännu.")
            }
            appendLine()
            appendLine("I databasen: $total registreringar")
            if (newest != null) {
                appendLine("Senaste svar: ${time(newest * 1000)}")
            }
            if (perMetric.isNotEmpty()) {
                appendLine()
                appendLine("Per metric:")
                perMetric.forEach { appendLine("  ${it.name}: ${it.count}") }
            }
        }
        runOnUiThread { status.text = text }
    }

    private fun requestNotificationPermissionIfNeeded() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) !=
            PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 1)
        }
    }

    private fun time(millis: Long): String =
        SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.ROOT).format(Date(millis))
}
