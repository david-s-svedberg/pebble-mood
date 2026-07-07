package se.svep.mood.companion

import android.content.Context
import android.util.Log
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import org.json.JSONObject
import se.svep.mood.companion.db.AppDatabase
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
 * Parses a pkjs export payload and stores it idempotently. Emits on
 * [lastImport] so any UI can refresh.
 */
class ImportRepository(context: Context) {

    private val db = AppDatabase.get(context)

    suspend fun import(body: String): ImportResult {
        val payload = JSONObject(body)
        val regs = payload.getJSONArray("registrations")
        val now = System.currentTimeMillis()

        val metrics = LinkedHashMap<Int, MetricEntity>()
        val rows = ArrayList<RegistrationEntity>(regs.length())
        var newest = 0L

        for (i in 0 until regs.length()) {
            val r = regs.getJSONObject(i)
            val metricId = r.getInt("metricId")
            val timestamp = r.getLong("timestamp")
            if (timestamp > newest) newest = timestamp

            metrics[metricId] = MetricEntity(
                metricId = metricId,
                name = r.optString("metricName", ""),
                type = r.optString("metricType", "unknown"),
                min = r.optInt("min", 0),
                max = r.optInt("max", 0),
                lastSeenAt = now / 1000,
            )
            rows.add(
                RegistrationEntity(
                    metricId = metricId,
                    groupId = r.optInt("groupId", 0),
                    groupName = r.optString("groupName", ""),
                    value = r.getInt("value"),
                    timestamp = timestamp,
                    importedAt = now,
                )
            )
        }

        db.metrics().upsertAll(metrics.values.toList())
        val inserted = db.registrations().insertAll(rows).count { it != -1L }

        val result = ImportResult(received = rows.size, new = inserted, ackedThrough = newest)
        Log.i(TAG, "import: received=${result.received} new=${result.new} ackedThrough=${result.ackedThrough}")
        lastImportFlow.value = result to now
        return result
    }

    companion object {
        private const val TAG = "MoodCompanion"

        private val lastImportFlow = MutableStateFlow<Pair<ImportResult, Long>?>(null)
        /** Latest completed import (result, wall-clock millis) — UI refresh signal. */
        val lastImport = lastImportFlow.asStateFlow()
    }
}
