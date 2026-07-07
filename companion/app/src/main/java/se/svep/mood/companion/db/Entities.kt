package se.svep.mood.companion.db

import androidx.room.Entity
import androidx.room.Index
import androidx.room.PrimaryKey

/**
 * Metric metadata as last seen in an import. Upserted every import — the watch
 * owns the definitions (until config parity, phase 4), we mirror them.
 */
@Entity(tableName = "metric")
data class MetricEntity(
    @PrimaryKey val metricId: Int,
    val name: String,
    /** "bool" | "interval" | "three_option" | "unknown" — as exported by pkjs. */
    val type: String,
    val min: Int,
    val max: Int,
    /** Unix seconds of the newest import that mentioned this metric. */
    val lastSeenAt: Long,
)

/**
 * One answered registration. The watch re-sends its full history on every
 * export until pruning kicks in, so the natural key (metricId, groupId,
 * timestamp) carries idempotence: re-imports are ignored, not duplicated.
 */
@Entity(
    tableName = "registration",
    indices = [Index(value = ["metricId", "groupId", "timestamp"], unique = true)],
)
data class RegistrationEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val metricId: Int,
    /** 0 = spontaneous, otherwise the scheduled group's id. */
    val groupId: Int,
    val groupName: String,
    val value: Int,
    /** Unix seconds (watch time of the answer; updated in place on re-answer). */
    val timestamp: Long,
    /** Unix millis when this row arrived here. */
    val importedAt: Long,
)
