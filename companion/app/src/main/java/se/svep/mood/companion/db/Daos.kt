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

    @Query("SELECT * FROM metric WHERE metricId = :id")
    suspend fun byId(id: Int): MetricEntity?

    @Query("UPDATE metric SET valence = :valence WHERE metricId = :id")
    suspend fun setValence(id: Int, valence: Int)
}

@Dao
interface GroupDao {
    @Upsert
    suspend fun upsertAll(groups: List<GroupEntity>)

    @Query("SELECT * FROM grp ORDER BY hour, minute")
    suspend fun all(): List<GroupEntity>

    @Query("DELETE FROM membership WHERE groupId = :groupId")
    suspend fun clearMembers(groupId: Int)

    @Insert(onConflict = OnConflictStrategy.IGNORE)
    suspend fun addMembers(members: List<MembershipEntity>)

    @Query("SELECT * FROM membership")
    suspend fun memberships(): List<MembershipEntity>
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

    /** Removes demo rows (DemoData marks them via importedAt). */
    @Query("DELETE FROM registration WHERE importedAt = :marker")
    suspend fun deleteByImportMarker(marker: Long): Int
}

data class MetricCount(val name: String, val count: Int)

@Dao
interface PendingDao {
    @Insert
    suspend fun enqueue(change: PendingChangeEntity): Long

    @Query("SELECT * FROM pending_change ORDER BY id")
    suspend fun all(): List<PendingChangeEntity>

    @Query("DELETE FROM pending_change WHERE id IN (:ids)")
    suspend fun delete(ids: List<Long>): Int
}
