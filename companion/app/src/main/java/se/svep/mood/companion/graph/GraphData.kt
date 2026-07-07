package se.svep.mood.companion.graph

import se.svep.mood.companion.db.MetricEntity
import se.svep.mood.companion.db.RegistrationEntity
import java.time.Instant
import java.time.LocalDate
import java.time.ZoneId

/**
 * Chart-ready data, resolution-agnostic: x is 0..1 across the window, y is
 * 0..1 normalized from the metric's own [min, max] (scales differ — Joy is
 * 1-5, Anxiety 0-5 — so series are indexed to a common base rather than
 * sharing a fake axis).
 */
data class ChartData(
    val lines: List<LineSeries>,
    val eventTracks: List<EventTrack>,
    /** Day boundaries as x positions 0..1, for gridlines + date labels. */
    val dayTicks: List<DayTick>,
)

data class DayTick(val x: Float, val date: LocalDate)

data class LinePoint(
    val x: Float,
    val yNorm: Float,
    val value: Float,
    val timestamp: Long,
)

data class LineSeries(
    val metric: MetricEntity,
    val points: List<LinePoint>,
)

/** Bool answers render as events on their own track — never as a line. */
data class EventPoint(
    val x: Float,
    /** true = yes (filled marker), false = explicit no (hollow marker). */
    val yes: Boolean,
    val timestamp: Long,
)

data class EventTrack(
    val metric: MetricEntity,
    val events: List<EventPoint>,
)

enum class Resolution { DAY, DETAIL }

/**
 * The day-aggregation semantics, shared by the graph's DAY mode and the
 * correlation engine (they MUST agree): group slots count once each (latest
 * answer per slot — a re-answer supersedes older imported versions),
 * spontaneous entries count individually; the day's value is the mean.
 * Bool day-value: 1 if ANY answer that day was yes, else 0.
 */
object DailyAggregation {
    fun dailyValue(dayRegs: List<RegistrationEntity>): Float {
        val slotValues = dayRegs
            .filter { it.groupId != 0 }
            .groupBy { it.groupId }
            .map { (_, slot) -> slot.maxBy { it.timestamp }.value.toFloat() }
        val spontaneous = dayRegs.filter { it.groupId == 0 }.map { it.value.toFloat() }
        val all = slotValues + spontaneous
        return all.sum() / all.size
    }

    fun boolDayValue(dayRegs: List<RegistrationEntity>): Float =
        if (dayRegs.any { it.value != 0 }) 1f else 0f
}

object GraphBuilder {

    /**
     * @param windowDays how many days back from (and including) today.
     * @param resolution DAY = one aggregated point per day; DETAIL = every
     *   registration at its actual timestamp (spontaneous entries visible).
     */
    fun build(
        metrics: List<MetricEntity>,
        registrations: List<RegistrationEntity>,
        windowDays: Int,
        resolution: Resolution,
        zone: ZoneId = ZoneId.systemDefault(),
    ): ChartData {
        val today = LocalDate.now(zone)
        val firstDay = today.minusDays((windowDays - 1).toLong())
        val windowStart = firstDay.atStartOfDay(zone).toInstant().epochSecond
        val windowEnd = today.plusDays(1).atStartOfDay(zone).toInstant().epochSecond
        val span = (windowEnd - windowStart).toFloat()

        fun xOf(timestamp: Long): Float =
            ((timestamp - windowStart) / span).coerceIn(0f, 1f)

        fun dayOf(timestamp: Long): LocalDate =
            Instant.ofEpochSecond(timestamp).atZone(zone).toLocalDate()

        // Day tick at each day's MIDPOINT so aggregated points sit centred in
        // their day column.
        val dayTicks = (0 until windowDays).map { i ->
            val date = firstDay.plusDays(i.toLong())
            val mid = date.atStartOfDay(zone).plusHours(12).toInstant().epochSecond
            DayTick(xOf(mid), date)
        }
        val dayMidX = dayTicks.associate { it.date to it.x }

        val byMetric = registrations
            .filter { it.timestamp in windowStart until windowEnd }
            .groupBy { it.metricId }

        val lines = ArrayList<LineSeries>()
        val tracks = ArrayList<EventTrack>()

        for (metric in metrics) {
            val regs = byMetric[metric.metricId].orEmpty()
            if (metric.type == "bool") {
                tracks.add(EventTrack(metric, buildEvents(regs, resolution, dayMidX, ::xOf, ::dayOf)))
            } else {
                lines.add(LineSeries(metric, buildLine(regs, resolution, dayMidX, metric, ::xOf, ::dayOf)))
            }
        }
        return ChartData(lines, tracks, dayTicks)
    }

    private fun buildLine(
        regs: List<RegistrationEntity>,
        resolution: Resolution,
        dayMidX: Map<LocalDate, Float>,
        metric: MetricEntity,
        xOf: (Long) -> Float,
        dayOf: (Long) -> LocalDate,
    ): List<LinePoint> {
        val range = (metric.max - metric.min).coerceAtLeast(1).toFloat()
        fun norm(value: Float) = ((value - metric.min) / range).coerceIn(0f, 1f)

        return when (resolution) {
            Resolution.DETAIL ->
                regs.sortedBy { it.timestamp }.map {
                    LinePoint(xOf(it.timestamp), norm(it.value.toFloat()), it.value.toFloat(), it.timestamp)
                }

            Resolution.DAY ->
                regs.groupBy { dayOf(it.timestamp) }.toSortedMap().mapNotNull { (date, dayRegs) ->
                    val x = dayMidX[date] ?: return@mapNotNull null
                    val value = dailyValue(dayRegs)
                    LinePoint(x, norm(value), value, dayRegs.maxOf { it.timestamp })
                }
        }
    }

    private fun dailyValue(dayRegs: List<RegistrationEntity>): Float =
        DailyAggregation.dailyValue(dayRegs)

    private fun buildEvents(
        regs: List<RegistrationEntity>,
        resolution: Resolution,
        dayMidX: Map<LocalDate, Float>,
        xOf: (Long) -> Float,
        dayOf: (Long) -> LocalDate,
    ): List<EventPoint> = when (resolution) {
        Resolution.DETAIL ->
            regs.sortedBy { it.timestamp }.map {
                EventPoint(xOf(it.timestamp), it.value != 0, it.timestamp)
            }

        // One marker per answered day: filled if ANY answer that day was yes
        // ("that day I did exercise"), hollow if the day was answered all-no.
        Resolution.DAY ->
            regs.groupBy { dayOf(it.timestamp) }.toSortedMap().mapNotNull { (date, dayRegs) ->
                val x = dayMidX[date] ?: return@mapNotNull null
                EventPoint(x, dayRegs.any { it.value != 0 }, dayRegs.maxOf { it.timestamp })
            }
    }
}
