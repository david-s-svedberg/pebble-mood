package se.svep.mood.companion

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.runBlocking
import se.svep.mood.companion.config.ConfigSync

/**
 * Foreground service that owns the import listener, so the sync happens the
 * moment the watch app runs — the companion UI doesn't need to be open.
 * Started from MainActivity; STICKY so the system restarts it if killed.
 */
class ImportService : Service() {

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private var server: ImportServer? = null
    private lateinit var repository: ImportRepository

    override fun onCreate() {
        super.onCreate()
        repository = ImportRepository(this)
        server = ImportServer(route = ::route).also { it.start() }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        startForegroundCompat()
        return START_STICKY
    }

    override fun onDestroy() {
        server?.stop()
        server = null
        scope.cancel()
        super.onDestroy()
    }

    override fun onBind(intent: Intent?): IBinder? = null

    // ImportServer calls from its socket thread; the DB work is quick and the
    // responses must carry results, so run to completion here.
    private fun route(method: String, path: String, body: String): String? = runBlocking {
        when {
            method == "POST" && path == "/import" -> {
                val result = repository.import(body)
                "{\"status\":\"ok\",\"received\":${result.received}," +
                    "\"new\":${result.new},\"ackedThrough\":${result.ackedThrough}}"
            }
            method == "GET" && path == "/pending" -> ConfigSync.pendingJson(this@ImportService)
            method == "POST" && path == "/pending-ack" -> ConfigSync.ack(this@ImportService, body)
            else -> null
        }
    }

    private fun startForegroundCompat() {
        val channelId = "import"
        val manager = getSystemService(NotificationManager::class.java)
        manager.createNotificationChannel(
            NotificationChannel(channelId, "Pebble-synk", NotificationManager.IMPORTANCE_MIN)
        )
        val notification: Notification = NotificationCompat.Builder(this, channelId)
            .setContentTitle("Mood Companion")
            .setContentText("Lyssnar efter synk från klockan")
            .setSmallIcon(android.R.drawable.stat_notify_sync_noanim)
            .setOngoing(true)
            .build()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.UPSIDE_DOWN_CAKE) {
            startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_SPECIAL_USE)
        } else {
            startForeground(NOTIFICATION_ID, notification)
        }
    }

    companion object {
        private const val NOTIFICATION_ID = 1

        fun start(context: Context) {
            ContextCompat.startForegroundService(context, Intent(context, ImportService::class.java))
        }
    }
}
