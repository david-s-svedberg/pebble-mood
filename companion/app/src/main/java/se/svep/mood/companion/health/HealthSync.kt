package se.svep.mood.companion.health

import android.content.Context
import androidx.work.CoroutineWorker
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.ExistingWorkPolicy
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkerParameters
import androidx.work.WorkManager
import java.util.concurrent.TimeUnit

/**
 * Background Health Connect refresh so steps/sleep/heart-rate stay current
 * without the user tapping "Hämta hälsodata". A worker pulls the last 30 days;
 * it no-ops (success) when HC is unavailable or permission was never granted,
 * so it never nags.
 */
class HealthSyncWorker(context: Context, params: WorkerParameters) :
    CoroutineWorker(context, params) {

    override suspend fun doWork(): Result {
        val manager = HealthConnectManager(applicationContext)
        if (!manager.isAvailable()) return Result.success()
        return try {
            if (manager.hasPermissions()) manager.importToRoom(30)
            Result.success()
        } catch (e: Exception) {
            Result.retry()
        }
    }
}

object HealthSync {
    private const val PERIODIC = "health-sync-periodic"
    private const val ONESHOT = "health-sync-now"

    /** Kicks off an immediate refresh and keeps a 6-hourly one going. */
    fun schedule(context: Context) {
        val wm = WorkManager.getInstance(context)
        wm.enqueueUniquePeriodicWork(
            PERIODIC,
            ExistingPeriodicWorkPolicy.KEEP,
            PeriodicWorkRequestBuilder<HealthSyncWorker>(6, TimeUnit.HOURS).build(),
        )
        wm.enqueueUniqueWork(
            ONESHOT,
            ExistingWorkPolicy.REPLACE,
            OneTimeWorkRequestBuilder<HealthSyncWorker>().build(),
        )
    }
}
