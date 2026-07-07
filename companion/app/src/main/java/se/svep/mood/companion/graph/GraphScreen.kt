package se.svep.mood.companion.graph

import android.content.Context
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.background
import androidx.compose.foundation.isSystemInDarkTheme
import androidx.compose.foundation.rememberScrollState as rememberVScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.FilterChip
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.SegmentedButton
import androidx.compose.material3.SegmentedButtonDefaults
import androidx.compose.material3.SingleChoiceSegmentedButtonRow
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.TextMeasurer
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.drawText
import androidx.compose.ui.text.rememberTextMeasurer
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import se.svep.mood.companion.ImportRepository
import se.svep.mood.companion.db.AppDatabase
import se.svep.mood.companion.db.MetricEntity
import java.time.Instant
import java.time.ZoneId
import java.time.format.DateTimeFormatter
import kotlin.math.abs

// Validated reference palette (dataviz skill): all eight categorical slots in
// their CVD-optimized fixed order, light + dark steps. The selection cap IS
// the palette size — colors are never cycled or generated (a 9th simultaneous
// series would be unidentifiable). Marks wear series color; text wears ink.
private val SERIES_LIGHT = listOf(
    Color(0xFF2A78D6), Color(0xFF1BAF7A), Color(0xFFEDA100), Color(0xFF008300),
    Color(0xFF4A3AA7), Color(0xFFE34948), Color(0xFFE87BA4), Color(0xFFEB6834),
)
private val SERIES_DARK = listOf(
    Color(0xFF3987E5), Color(0xFF199E70), Color(0xFFC98500), Color(0xFF008300),
    Color(0xFF9085E9), Color(0xFFE66767), Color(0xFFD55181), Color(0xFFD95926),
)
private val MAX_SELECTED = SERIES_LIGHT.size

private data class Chrome(
    val surface: Color,
    val inkPrimary: Color,
    val inkSecondary: Color,
    val inkMuted: Color,
    val gridline: Color,
    val baseline: Color,
)

private val CHROME_LIGHT = Chrome(
    surface = Color(0xFFFCFCFB), inkPrimary = Color(0xFF0B0B0B), inkSecondary = Color(0xFF52514E),
    inkMuted = Color(0xFF898781), gridline = Color(0xFFE1E0D9), baseline = Color(0xFFC3C2B7),
)
private val CHROME_DARK = Chrome(
    surface = Color(0xFF1A1A19), inkPrimary = Color(0xFFFFFFFF), inkSecondary = Color(0xFFC3C2B7),
    inkMuted = Color(0xFF898781), gridline = Color(0xFF2C2C2A), baseline = Color(0xFF383835),
)

/** Sticky metric→color-slot assignment: survivors keep their color when the
 *  selection changes (color follows the entity, not its rank). Persisted. */
private class ColorSlots(context: Context) {
    private val prefs = context.getSharedPreferences("graph", Context.MODE_PRIVATE)
    private val map = LinkedHashMap<Int, Int>()

    init {
        prefs.getString("slots", "")!!.split(',').filter { it.contains(':') }.forEach {
            val (id, slot) = it.split(':')
            map[id.toInt()] = slot.toInt()
        }
    }

    // Safe fallback rather than throw: a recomposition can briefly draw stale
    // chart data after a deselect (the async rebuild lands a frame later).
    fun slotFor(metricId: Int): Int = map[metricId] ?: 0

    fun select(metricId: Int): Boolean {
        if (map.containsKey(metricId)) return true
        if (map.size >= MAX_SELECTED) return false
        val free = (0 until MAX_SELECTED).first { slot -> slot !in map.values }
        map[metricId] = free
        persist()
        return true
    }

    fun deselect(metricId: Int) {
        map.remove(metricId)
        persist()
    }

    fun selected(): Set<Int> = map.keys.toSet()

    private fun persist() {
        prefs.edit().putString("slots", map.entries.joinToString(",") { "${it.key}:${it.value}" }).apply()
    }
}

@Composable
fun GraphScreen(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val dark = isSystemInDarkTheme()
    val chrome = if (dark) CHROME_DARK else CHROME_LIGHT
    val seriesColors = if (dark) SERIES_DARK else SERIES_LIGHT

    val slots = remember { ColorSlots(context) }
    var selected by remember { mutableStateOf(slots.selected()) }
    var windowDays by remember { mutableStateOf(7) }
    var resolution by remember { mutableStateOf(Resolution.DAY) }

    var metrics by remember { mutableStateOf<List<MetricEntity>>(emptyList()) }
    var chart by remember { mutableStateOf<ChartData?>(null) }
    // Colors captured TOGETHER with the chart data (same snapshot) — looking
    // them up live while stale data draws is a crash/mismatch race.
    var chartColors by remember { mutableStateOf<Map<Int, Color>>(emptyMap()) }
    var readout by remember { mutableStateOf<String?>(null) }

    val dataVersion by ImportRepository.dataVersion.collectAsStateSafe()

    LaunchedEffect(selected, windowDays, resolution, dataVersion) {
        val colorSnapshot = selected.associateWith { seriesColors[slots.slotFor(it)] }
        withContext(Dispatchers.IO) {
            val db = AppDatabase.get(context)
            metrics = db.metrics().all()
            val chosen = metrics.filter { it.metricId in selected }
            val built = if (chosen.isEmpty()) null else {
                val from = System.currentTimeMillis() / 1000 - windowDays * 86_400L
                val regs = db.registrations().inWindow(chosen.map { it.metricId }, from)
                GraphBuilder.build(chosen, regs, windowDays, resolution)
            }
            chart = built
            chartColors = colorSnapshot
        }
    }

    Column(modifier = modifier.verticalScroll(rememberVScrollState()).padding(12.dp)) {
        // Filter row: metric chips (scrollable), then window + resolution.
        Row(Modifier.horizontalScroll(rememberScrollState())) {
            metrics.forEach { metric ->
                val isSelected = metric.metricId in selected
                FilterChip(
                    selected = isSelected,
                    onClick = {
                        if (isSelected) slots.deselect(metric.metricId)
                        else slots.select(metric.metricId)
                        selected = slots.selected()
                    },
                    label = { Text(metric.name) },
                    leadingIcon = if (isSelected) ({
                        Spacer(
                            Modifier.size(10.dp)
                                .background(seriesColors[slots.slotFor(metric.metricId)], CircleShape)
                        )
                    }) else null,
                    modifier = Modifier.padding(end = 6.dp),
                )
            }
        }
        Row(
            Modifier.fillMaxWidth().padding(vertical = 8.dp),
            horizontalArrangement = Arrangement.spacedBy(8.dp),
            verticalAlignment = Alignment.CenterVertically,
        ) {
            SingleChoiceSegmentedButtonRow {
                listOf(7, 30).forEachIndexed { i, days ->
                    SegmentedButton(
                        selected = windowDays == days,
                        onClick = { windowDays = days },
                        shape = SegmentedButtonDefaults.itemShape(i, 2),
                    ) { Text("$days d") }
                }
            }
            SingleChoiceSegmentedButtonRow {
                Resolution.entries.forEachIndexed { i, res ->
                    SegmentedButton(
                        selected = resolution == res,
                        onClick = { resolution = res },
                        shape = SegmentedButtonDefaults.itemShape(i, 2),
                    ) { Text(if (res == Resolution.DAY) "Dag" else "Detalj") }
                }
            }
        }

        readout?.let {
            Text(it, style = MaterialTheme.typography.bodyMedium, color = chrome.inkPrimary)
            Spacer(Modifier.height(4.dp))
        }

        val data = chart
        if (data == null) {
            Text(
                if (selected.isEmpty()) "Välj metrics ovan (max $MAX_SELECTED)."
                else "Laddar…",
                color = chrome.inkSecondary,
            )
        } else {
            Chart(
                data = data,
                colors = chartColors,
                chrome = chrome,
                windowDays = windowDays,
                onReadout = { readout = it },
            )
            Spacer(Modifier.height(8.dp))
            Legend(data, chartColors, chrome)
        }
    }
}

/** StateFlow.collectAsState without adding the runtime-compose dependency. */
@Composable
private fun <T> kotlinx.coroutines.flow.StateFlow<T>.collectAsStateSafe(): androidx.compose.runtime.State<T> {
    val state = remember { mutableStateOf(value) }
    LaunchedEffect(this) { collect { state.value = it } }
    return state
}

@Composable
private fun Legend(
    data: ChartData,
    colors: Map<Int, Color>,
    chrome: Chrome,
) {
    Column {
        (data.lines.map { it.metric } + data.eventTracks.map { it.metric }).forEach { metric ->
            Row(verticalAlignment = Alignment.CenterVertically, modifier = Modifier.padding(vertical = 2.dp)) {
                Spacer(Modifier.size(10.dp).background(colors[metric.metricId] ?: chrome.inkMuted, CircleShape))
                Spacer(Modifier.width(6.dp))
                val scale = if (metric.type == "bool") "ja/nej" else "${metric.min}–${metric.max}"
                Text("${metric.name} ($scale)", color = chrome.inkSecondary, fontSize = 13.sp)
            }
        }
    }
}

@Composable
private fun Chart(
    data: ChartData,
    colors: Map<Int, Color>,
    chrome: Chrome,
    windowDays: Int,
    onReadout: (String?) -> Unit,
) {
    val measurer = rememberTextMeasurer()
    val density = LocalDensity.current
    val trackHeight = with(density) { 26.dp.toPx() }
    val plotHeightDp = 220.dp
    val chartHeightDp = plotHeightDp + with(density) { (data.eventTracks.size * 26).dp } + 24.dp

    Canvas(
        modifier = Modifier
            .fillMaxWidth()
            .height(chartHeightDp)
            .pointerInput(data) {
                detectTapGestures { tap ->
                    onReadout(nearestReadout(data, tap, size.width.toFloat(), size.height.toFloat(), trackHeight))
                }
            }
    ) {
        val labelStyle = TextStyle(fontSize = 10.sp, color = chrome.inkMuted)
        val axisPad = 34f                    // room for date labels at the bottom
        val plotH = size.height - data.eventTracks.size * trackHeight - axisPad
        val w = size.width

        drawRoundRect(chrome.surface, cornerRadius = CornerRadius(12f, 12f))

        // Day gridlines (hairline) + date labels (every day at 7d, sparser at 30d).
        val labelEvery = if (windowDays <= 7) 1 else 5
        data.dayTicks.forEachIndexed { i, tick ->
            val x = tick.x * w
            drawLine(chrome.gridline, Offset(x, 8f), Offset(x, plotH), 1f)
            if (i % labelEvery == 0) {
                val text = tick.date.format(DateTimeFormatter.ofPattern("d/M"))
                val layout = measurer.measure(text, labelStyle)
                drawText(layout, topLeft = Offset(x - layout.size.width / 2f, size.height - axisPad + 6f))
            }
        }
        // Baseline under the plot.
        drawLine(chrome.baseline, Offset(0f, plotH), Offset(w, plotH), 2f)

        // Interval/three-option series: 2px lines + 8dp markers, normalized y.
        data.lines.forEach { series ->
            val color = colors[series.metric.metricId] ?: chrome.inkMuted
            fun yOf(p: LinePoint) = 12f + (1f - p.yNorm) * (plotH - 24f)

            if (series.points.size > 1) {
                val path = Path()
                series.points.forEachIndexed { i, p ->
                    if (i == 0) path.moveTo(p.x * w, yOf(p)) else path.lineTo(p.x * w, yOf(p))
                }
                drawPath(path, color, style = Stroke(width = 2.dp.toPx()))
            }
            series.points.forEach { p ->
                // 2px surface ring so overlapping marks stay separable.
                drawCircle(chrome.surface, radius = 5.dp.toPx(), center = Offset(p.x * w, yOf(p)))
                drawCircle(color, radius = 4.dp.toPx(), center = Offset(p.x * w, yOf(p)))
            }
            // Direct label at the line's end — text in ink, not series color.
            // Only when an actual line exists: single points cluster at the
            // same spot (one day of data) and the labels pile up unreadably —
            // the legend carries identity there.
            if (series.points.size >= 2) {
                val last = series.points.last()
                val layout = measurer.measure(series.metric.name, TextStyle(fontSize = 11.sp, color = chrome.inkSecondary))
                val lx = (last.x * w - layout.size.width - 8f).coerceAtLeast(2f)
                drawText(layout, topLeft = Offset(lx, (yOf(last) - layout.size.height - 6f).coerceAtLeast(0f)))
            }
        }

        // Bool metrics: event tracks under the plot — markers, never lines.
        data.eventTracks.forEachIndexed { trackIndex, track ->
            val color = colors[track.metric.metricId] ?: chrome.inkMuted
            val cy = plotH + trackIndex * trackHeight + trackHeight / 2f
            drawLine(chrome.gridline, Offset(0f, cy), Offset(w, cy), 1f)
            val layout = measurer.measure(track.metric.name, TextStyle(fontSize = 10.sp, color = chrome.inkSecondary))
            drawText(layout, topLeft = Offset(4f, cy - trackHeight / 2f + 2f))

            track.events.forEach { e ->
                val center = Offset(e.x * w, cy)
                if (e.yes) {
                    drawCircle(chrome.surface, radius = 6.dp.toPx(), center = center)
                    drawCircle(color, radius = 5.dp.toPx(), center = center)
                } else {
                    drawCircle(chrome.surface, radius = 6.dp.toPx(), center = center)
                    drawCircle(color, radius = 5.dp.toPx(), center = center, style = Stroke(width = 2.dp.toPx()))
                }
            }
        }
    }
}

/** Tap = mobile hover: find the nearest mark and describe it. */
private fun nearestReadout(
    data: ChartData,
    tap: Offset,
    w: Float,
    h: Float,
    trackHeight: Float,
): String? {
    val axisPad = 34f
    val plotH = h - data.eventTracks.size * trackHeight - axisPad
    var best: Pair<Float, String>? = null

    fun consider(distance: Float, label: String) {
        if (distance < (best?.first ?: Float.MAX_VALUE)) best = distance to label
    }

    val timeFormat = DateTimeFormatter.ofPattern("d/M HH:mm")
    fun timeOf(ts: Long) =
        Instant.ofEpochSecond(ts).atZone(ZoneId.systemDefault()).format(timeFormat)

    data.lines.forEach { series ->
        series.points.forEach { p ->
            val px = p.x * w
            val py = 12f + (1f - p.yNorm) * (plotH - 24f)
            val d = abs(px - tap.x) + abs(py - tap.y)
            val value = if (p.value == p.value.toLong().toFloat()) p.value.toLong().toString()
                        else "%.1f".format(p.value)
            consider(d, "${series.metric.name}: $value  (${timeOf(p.timestamp)})")
        }
    }
    data.eventTracks.forEachIndexed { trackIndex, track ->
        val cy = plotH + trackIndex * trackHeight + trackHeight / 2f
        track.events.forEach { e ->
            val d = abs(e.x * w - tap.x) + abs(cy - tap.y)
            consider(d, "${track.metric.name}: ${if (e.yes) "ja" else "nej"}  (${timeOf(e.timestamp)})")
        }
    }
    return best?.takeIf { it.first < 120f }?.second
}
