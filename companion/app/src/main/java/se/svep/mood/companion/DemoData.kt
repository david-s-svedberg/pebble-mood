package se.svep.mood.companion

import android.content.Context
import se.svep.mood.companion.db.AppDatabase
import se.svep.mood.companion.db.MetricEntity
import se.svep.mood.companion.db.RegistrationEntity
import java.time.LocalDate
import java.time.ZoneId
import kotlin.math.roundToInt
import kotlin.math.sin
import kotlin.random.Random

/**
 * TEMPORARY demo data so the graphs can be evaluated before real history has
 * accumulated. Rows are marked with importedAt = DEMO_MARKER and removed by
 * [clear] — real imports are never touched. Metric ids mirror the watch seed
 * (Joy=1, Anxiety=2, Irritation=3, Stress=4, Exercised=8) so demo rows merge
 * naturally with real ones instead of creating parallel metrics.
 */
object DemoData {

    const val DEMO_MARKER = -1L
    private const val DAYS = 30

    suspend fun seed(context: Context): Int {
        val db = AppDatabase.get(context)
        val zone = ZoneId.systemDefault()
        val random = Random(42)
        val now = System.currentTimeMillis() / 1000

        db.metrics().upsertAll(
            listOf(
                MetricEntity(1, "Joy", "interval", 1, 5, now),
                MetricEntity(2, "Anxiety", "interval", 0, 5, now),
                MetricEntity(3, "Irritation", "interval", 0, 5, now),
                MetricEntity(4, "Stress", "interval", 0, 5, now),
                MetricEntity(8, "Exercised", "bool", 0, 1, now),
            )
        )

        val rows = ArrayList<RegistrationEntity>()
        val today = LocalDate.now(zone)

        fun at(date: LocalDate, hour: Int, minute: Int): Long =
            date.atTime(hour, minute).atZone(zone).toEpochSecond()

        fun add(metricId: Int, groupId: Int, groupName: String, value: Int, ts: Long) {
            rows.add(
                RegistrationEntity(
                    metricId = metricId, groupId = groupId, groupName = groupName,
                    value = value, timestamp = ts, importedAt = DEMO_MARKER,
                )
            )
        }

        for (i in (DAYS - 1) downTo 1) {   // skip today: real data lives there
            val date = today.minusDays(i.toLong())
            val weekRhythm = sin(i / 7.0 * 2 * Math.PI)   // weekly mood wave
            val exercisedYesterday = (i + 1) % 3 == 0

            // Joy 1-5: base 3, weekly wave, small boost the day after exercise.
            val joyBase = 3.0 + weekRhythm + (if (exercisedYesterday) 0.7 else 0.0)
            fun joy() = (joyBase + random.nextDouble(-0.8, 0.8)).roundToInt().coerceIn(1, 5)
            if (random.nextInt(10) != 0) add(1, 1, "Morning", joy(), at(date, 7, 31 + random.nextInt(20)))
            if (random.nextInt(10) != 0) add(1, 3, "Evening", joy(), at(date, 21, 31 + random.nextInt(20)))
            // Occasional spontaneous Joy (detail-mode showcase).
            if (random.nextInt(4) == 0) add(1, 0, "", joy(), at(date, 12 + random.nextInt(8), random.nextInt(60)))

            // Anxiety 0-5: roughly inverse of joy.
            val anxiety = (4.0 - joyBase + random.nextDouble(-0.7, 0.7)).roundToInt().coerceIn(0, 5)
            if (random.nextInt(8) != 0) add(2, 3, "Evening", anxiety, at(date, 21, 33 + random.nextInt(20)))

            // Irritation 0-5: low with random spikes; Stress follows it loosely.
            val spike = if (random.nextInt(6) == 0) 3 else 0
            val irritation = (1 + spike + random.nextInt(-1, 2)).coerceIn(0, 5)
            add(3, 2, "Lunch", irritation, at(date, 12, 31 + random.nextInt(15)))
            val stress = (irritation + random.nextInt(-1, 2)).coerceIn(0, 5)
            add(4, 2, "Lunch", stress, at(date, 12, 32 + random.nextInt(15)))
            if (random.nextInt(3) == 0) add(4, 3, "Evening", (stress + random.nextInt(-1, 2)).coerceIn(0, 5), at(date, 21, 35))

            // Exercised: yes every ~3rd day, sometimes an explicit no (hollow marker).
            when {
                i % 3 == 0 -> add(8, 3, "Evening", 1, at(date, 21, 34))
                random.nextInt(3) == 0 -> add(8, 3, "Evening", 0, at(date, 21, 34))
            }
        }

        return db.registrations().insertAll(rows).count { it != -1L }
    }

    suspend fun clear(context: Context): Int =
        AppDatabase.get(context).registrations().deleteByImportMarker(DEMO_MARKER)
}
