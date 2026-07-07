package se.svep.mood.companion.db

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase

@Database(
    entities = [
        MetricEntity::class,
        GroupEntity::class,
        MembershipEntity::class,
        RegistrationEntity::class,
        PendingChangeEntity::class,
    ],
    version = 3,
    exportSchema = false,
)
abstract class AppDatabase : RoomDatabase() {
    abstract fun metrics(): MetricDao
    abstract fun groups(): GroupDao
    abstract fun registrations(): RegistrationDao
    abstract fun pending(): PendingDao

    companion object {
        @Volatile private var instance: AppDatabase? = null

        fun get(context: Context): AppDatabase =
            instance ?: synchronized(this) {
                instance ?: Room.databaseBuilder(
                    context.applicationContext,
                    AppDatabase::class.java,
                    "mood.db",
                )
                    // Pre-release: schema changes reset the DB. Acceptable —
                    // the watch re-exports its retained history on next sync
                    // (only pruned-away days would be lost; none exist yet).
                    .fallbackToDestructiveMigration()
                    .build().also { instance = it }
            }
    }
}
