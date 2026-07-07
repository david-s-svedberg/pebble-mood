package se.svep.mood.companion.db

import androidx.room.Entity
import androidx.room.Index
import androidx.room.PrimaryKey

/**
 * Metric definition as last seen in a config export. The watch owns the
 * definitions (until phase-4 editing ships both ways); we mirror them —
 * EXCEPT [valence], which is companion-only metadata and survives imports.
 */
@Entity(tableName = "metric")
data class MetricEntity(
    @PrimaryKey val metricId: Int,
    val name: String,
    /** "bool" | "interval" | "three_option" | "unknown" — as exported by pkjs. */
    val type: String,
    val min: Int,
    val max: Int,
    /** IconChoice ordinal on the watch (0 = none). */
    val mainIcon: Int = 0,
    /** Comma-joined IconChoice ordinals for the option buttons. */
    val optionIcons: String = "",
    /** Unit-separator-joined option labels. */
    val optionTexts: String = "",
    /**
     * Companion-only: is a high value good (+1), bad (-1) or neutral (0)?
     * Used to frame insights ("seems good for you"), never by the math.
     */
    val valence: Int = 0,
    /** Unix seconds of the newest import that mentioned this metric. */
    val lastSeenAt: Long,
)

/** Scheduled group (registration slot) as exported by the watch. */
@Entity(tableName = "grp")
data class GroupEntity(
    @PrimaryKey val groupId: Int,
    val name: String,
    val hour: Int,
    val minute: Int,
    val active: Boolean,
)

/** Which metrics belong to which group (mirrors the watch's GroupMetric). */
@Entity(tableName = "membership", primaryKeys = ["groupId", "metricId"])
data class MembershipEntity(
    val groupId: Int,
    val metricId: Int,
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

/**
 * A config edit made in the companion, queued until pkjs pulls and applies it
 * to the watch (the watch must be running for AppMessage). payload is the
 * change as JSON in the pkjs wire shape (kind: "group" | "metric").
 */
@Entity(tableName = "pending_change")
data class PendingChangeEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val kind: String,
    val payload: String,
    val createdAt: Long,
)
