package se.svep.mood.companion.insights

import se.svep.mood.companion.db.MetricEntity
import se.svep.mood.companion.db.RegistrationEntity
import se.svep.mood.companion.graph.DailyAggregation
import java.time.Instant
import java.time.LocalDate
import java.time.ZoneId
import kotlin.math.abs
import kotlin.math.sqrt

/**
 * The built-in correlation engine (phase 7, math half): Spearman rank
 * correlation over per-day series, pairwise across ALL metrics with data —
 * generic, nothing hardcoded about the seeded set. Lag 0 ("same day") plus
 * lag 1 in both directions ("X predicts Y the next day").
 *
 * Spearman rather than Pearson: our scales are small ordinal ranges (0-5,
 * bools) and relationships need only be monotonic, not linear. Ties get
 * average ranks.
 */
object Correlations {

    data class Insight(
        val a: MetricEntity,
        val b: MetricEntity,
        /** 0 = same day; 1 = a's value predicts b's value the NEXT day. */
        val lagDays: Int,
        val r: Double,
        /** Overlapping day count the estimate rests on. */
        val n: Int,
    )

    private const val MIN_OVERLAP_DAYS = 7
    private const val MIN_ABS_R = 0.25

    fun compute(
        metrics: List<MetricEntity>,
        registrations: List<RegistrationEntity>,
        zone: ZoneId = ZoneId.systemDefault(),
    ): List<Insight> {
        val series: Map<Int, Map<LocalDate, Double>> = metrics.associate { metric ->
            metric.metricId to daySeries(metric, registrations, zone)
        }
        val withData = metrics.filter { (series[it.metricId]?.size ?: 0) >= MIN_OVERLAP_DAYS }

        val insights = ArrayList<Insight>()
        for (i in withData.indices) {
            for (j in withData.indices) {
                val a = withData[i]
                val b = withData[j]
                val sa = series.getValue(a.metricId)
                val sb = series.getValue(b.metricId)

                if (i < j) {
                    correlate(sa, sb, 0)?.let { (r, n) -> insights.add(Insight(a, b, 0, r, n)) }
                }
                if (i != j) {
                    correlate(sa, sb, 1)?.let { (r, n) -> insights.add(Insight(a, b, 1, r, n)) }
                }
            }
        }
        return insights
            .filter { abs(it.r) >= MIN_ABS_R }
            .sortedByDescending { abs(it.r) }
    }

    private fun daySeries(
        metric: MetricEntity,
        registrations: List<RegistrationEntity>,
        zone: ZoneId,
    ): Map<LocalDate, Double> =
        registrations
            .filter { it.metricId == metric.metricId }
            .groupBy { Instant.ofEpochSecond(it.timestamp).atZone(zone).toLocalDate() }
            .mapValues { (_, dayRegs) ->
                if (metric.type == "bool") DailyAggregation.boolDayValue(dayRegs).toDouble()
                else DailyAggregation.dailyValue(dayRegs).toDouble()
            }

    /** Aligns the series at the given lag (a's day d against b's day d+lag). */
    private fun correlate(
        a: Map<LocalDate, Double>,
        b: Map<LocalDate, Double>,
        lagDays: Int,
    ): Pair<Double, Int>? {
        val xs = ArrayList<Double>()
        val ys = ArrayList<Double>()
        for ((day, valueA) in a) {
            val valueB = b[day.plusDays(lagDays.toLong())] ?: continue
            xs.add(valueA)
            ys.add(valueB)
        }
        if (xs.size < MIN_OVERLAP_DAYS) return null
        val r = spearman(xs, ys) ?: return null
        return r to xs.size
    }

    /** Spearman = Pearson on average ranks. Null when a series is constant. */
    private fun spearman(xs: List<Double>, ys: List<Double>): Double? =
        pearson(ranks(xs), ranks(ys))

    private fun ranks(values: List<Double>): DoubleArray {
        val indexed = values.withIndex().sortedBy { it.value }
        val ranks = DoubleArray(values.size)
        var i = 0
        while (i < indexed.size) {
            var j = i
            while (j + 1 < indexed.size && indexed[j + 1].value == indexed[i].value) j++
            val averageRank = (i + j) / 2.0 + 1.0
            for (k in i..j) ranks[indexed[k].index] = averageRank
            i = j + 1
        }
        return ranks
    }

    private fun pearson(xs: DoubleArray, ys: DoubleArray): Double? {
        val n = xs.size
        val meanX = xs.average()
        val meanY = ys.average()
        var cov = 0.0
        var varX = 0.0
        var varY = 0.0
        for (i in 0 until n) {
            val dx = xs[i] - meanX
            val dy = ys[i] - meanY
            cov += dx * dy
            varX += dx * dx
            varY += dy * dy
        }
        if (varX == 0.0 || varY == 0.0) return null   // constant series
        return cov / sqrt(varX * varY)
    }
}
