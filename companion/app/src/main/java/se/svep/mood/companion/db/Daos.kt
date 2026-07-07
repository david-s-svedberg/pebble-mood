package se.svep.mood.companion.db

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Upsert

@Dao
interface MetricDao {
    @Upsert
    suspend fun upsertAll(metrics: List<MetricEntity>)

    @Query("SELECT * FROM metric ORDER BY metricId")
    suspend fun all(): List<MetricEntity>
}

@Dao
interface RegistrationDao {
    /**
     * IGNORE + the unique (metricId, groupId, timestamp) index = idempotent
     * import: the watch re-sends its full history every export. Returns the
     * rowid per item, -1 for ignored duplicates.
     *
     * Note: a group slot re-answered on the watch gets a NEW timestamp, so the
     * older answer already imported stays as a historical row — analysis
     * aggregates "latest per slot per day" (phase 3/7).
     */
    @Insert(onConflict = OnConflictStrategy.IGNORE)
    suspend fun insertAll(registrations: List<RegistrationEntity>): List<Long>

    @Query("SELECT COUNT(*) FROM registration")
    suspend fun count(): Int

    @Query("SELECT MAX(timestamp) FROM registration")
    suspend fun newestTimestamp(): Long?

    @Query(
        "SELECT m.name AS name, COUNT(r.id) AS count " +
        "FROM registration r JOIN metric m ON m.metricId = r.metricId " +
        "GROUP BY r.metricId ORDER BY count DESC"
    )
    suspend fun countsPerMetric(): List<MetricCount>

    @Query(
        "SELECT * FROM registration " +
        "WHERE metricId IN (:metricIds) AND timestamp >= :fromTimestamp " +
        "ORDER BY timestamp"
    )
    suspend fun inWindow(metricIds: List<Int>, fromTimestamp: Long): List<RegistrationEntity>
}

data class MetricCount(val name: String, val count: Int)
