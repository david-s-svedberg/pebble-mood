package se.svep.mood.companion.health

import android.content.Context
import android.util.Log
import androidx.activity.result.contract.ActivityResultContract
import androidx.health.connect.client.HealthConnectClient
import androidx.health.connect.client.PermissionController
import androidx.health.connect.client.permission.HealthPermission
import androidx.health.connect.client.records.HeartRateRecord
import androidx.health.connect.client.records.SleepSessionRecord
import androidx.health.connect.client.records.StepsRecord
import androidx.health.connect.client.request.AggregateGroupByPeriodRequest
import androidx.health.connect.client.time.TimeRangeFilter
import se.svep.mood.companion.ImportRepository
import se.svep.mood.companion.db.AppDatabase
import se.svep.mood.companion.db.MetricEntity
import se.svep.mood.companion.db.RegistrationEntity
import java.time.LocalDate
import java.time.LocalDateTime
import java.time.Period
import java.time.ZoneId

/**
 * Synthetic auto-metrics for Health Connect data. Ids sit in a high range so
 * they never collide with the watch's own metrics (which count up from 1) —
 * the watch has no idea these exist, and its config export never touches them.
 * They're analysis-only: not in any group, so they never appear in the phone
 * answer flow, but the graph + Spearman engine pick them up like any metric.
 */
object HealthMetrics {
    const val STEPS = 9001
    const val SLEEP = 9002   // minutes
    const val HEART = 9003   // avg bpm
    val ALL = listOf(STEPS, SLEEP, HEART)
}

data class HealthImportResult(val days: Int, val steps: Int, val sleep: Int, val heart: Int) {
    val total get() = steps + sleep + heart
}

/**
 * Thin wrapper over the Health Connect client: availability, the read-permission
 * set, and a per-day aggregation of steps/sleep/heart-rate into Room.
 */
class HealthConnectManager(private val context: Context) {

    private val client by lazy { HealthConnectClient.getOrCreate(context) }

    val permissions: Set<String> = setOf(
        HealthPermission.getReadPermission(StepsRecord::class),
        HealthPermission.getReadPermission(SleepSessionRecord::class),
        HealthPermission.getReadPermission(HeartRateRecord::class),
    )

    /** SDK_UNAVAILABLE / SDK_UNAVAILABLE_PROVIDER_UPDATE_REQUIRED / SDK_AVAILABLE. */
    fun sdkStatus(): Int = HealthConnectClient.getSdkStatus(context)

    fun isAvailable(): Boolean = sdkStatus() == HealthConnectClient.SDK_AVAILABLE

    suspend fun hasPermissions(): Boolean =
        client.permissionController.getGrantedPermissions().containsAll(permissions)

    fun requestPermissionsContract(): ActivityResultContract<Set<String>, Set<String>> =
        PermissionController.createRequestPermissionResultContract()

    /**
     * Reads the last [days] days of steps/sleep/heart-rate, aggregated per local
     * day, and refreshes them in Room (full replace of the health rows — cheap,
     * and correct when today's totals grow as more data arrives). Returns how
     * many day-rows each metric produced.
     */
    suspend fun importToRoom(days: Int = 30): HealthImportResult {
        val zone = ZoneId.systemDefault()
        val end = LocalDateTime.now(zone)
        val start = LocalDate.now(zone).minusDays(days.toLong()).atStartOfDay()
        val range = TimeRangeFilter.between(start, end)

        val stepRows = aggregateDaily(StepsRecord.COUNT_TOTAL, range, HealthMetrics.STEPS, zone) {
            it.toInt()
        }
        val sleepRows = aggregateDaily(SleepSessionRecord.SLEEP_DURATION_TOTAL, range, HealthMetrics.SLEEP, zone) {
            it.toMinutes().toInt()
        }
        val heartRows = aggregateDaily(HeartRateRecord.BPM_AVG, range, HealthMetrics.HEART, zone) {
            it.toInt()
        }

        val db = AppDatabase.get(context)
        val now = System.currentTimeMillis()
        upsertMetricDefs(stepRows.isNotEmpty(), sleepRows.isNotEmpty(), heartRows.isNotEmpty(), now)

        // Full refresh: drop the previous health rows, insert the fresh window.
        db.registrations().deleteByMetricIds(HealthMetrics.ALL)
        db.registrations().insertAll(stepRows + sleepRows + heartRows)

        ImportRepository.bumpDataVersion()
        val result = HealthImportResult(days, stepRows.size, sleepRows.size, heartRows.size)
        Log.i(TAG, "health import: $result")
        return result
    }

    /**
     * Aggregates one metric into per-day RegistrationEntity rows. Generic over
     * the metric's aggregate value type via [convert] (Long steps, Duration
     * sleep, Long bpm all funnel to an Int day value).
     */
    private suspend fun <T : Any> aggregateDaily(
        metric: androidx.health.connect.client.aggregate.AggregateMetric<T>,
        range: TimeRangeFilter,
        metricId: Int,
        zone: ZoneId,
        convert: (T) -> Int,
    ): List<RegistrationEntity> {
        val buckets = client.aggregateGroupByPeriod(
            AggregateGroupByPeriodRequest(
                metrics = setOf(metric),
                timeRangeFilter = range,
                timeRangeSlicer = Period.ofDays(1),
            )
        )
        val now = System.currentTimeMillis()
        return buckets.mapNotNull { bucket ->
            val value = bucket.result[metric]?.let(convert) ?: return@mapNotNull null
            val dayStart = bucket.startTime.toLocalDate().atStartOfDay(zone).toEpochSecond()
            RegistrationEntity(
                metricId = metricId,
                groupId = 0,
                groupName = "Hälsa",
                value = value,
                timestamp = dayStart,
                importedAt = now,
            )
        }
    }

    /** Creates/refreshes the auto-metric definitions, preserving user valence. */
    private suspend fun upsertMetricDefs(steps: Boolean, sleep: Boolean, heart: Boolean, now: Long) {
        val db = AppDatabase.get(context)
        val rows = ArrayList<MetricEntity>()
        // valence defaults: more steps/sleep good (+1); higher avg HR worse (-1).
        if (steps) rows.add(healthMetric(HealthMetrics.STEPS, "Steg", 0, 15000, +1, now))
        if (sleep) rows.add(healthMetric(HealthMetrics.SLEEP, "Sömn (min)", 0, 600, +1, now))
        if (heart) rows.add(healthMetric(HealthMetrics.HEART, "Puls (snitt)", 40, 100, -1, now))
        if (rows.isNotEmpty()) db.metrics().upsertAll(rows)
    }

    private suspend fun healthMetric(id: Int, name: String, min: Int, max: Int, defaultValence: Int, now: Long): MetricEntity {
        val existingValence = AppDatabase.get(context).metrics().byId(id)?.valence ?: defaultValence
        return MetricEntity(
            metricId = id,
            name = name,
            type = "interval",
            min = min,
            max = max,
            valence = existingValence,
            lastSeenAt = now / 1000,
        )
    }

    private companion object {
        const val TAG = "MoodHealth"
    }
}
