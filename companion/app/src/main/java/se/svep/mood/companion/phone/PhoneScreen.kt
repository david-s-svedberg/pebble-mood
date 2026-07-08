package se.svep.mood.companion.phone

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.Divider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import se.svep.mood.companion.ImportRepository
import se.svep.mood.companion.config.ConfigSync
import se.svep.mood.companion.db.AppDatabase
import se.svep.mood.companion.db.GroupEntity
import se.svep.mood.companion.db.MetricEntity
import java.util.Calendar

private const val UNIT_SEP = ""

/** A group plus its member metrics and which are already answered today. */
private data class DuePass(
    val group: GroupEntity,
    val metrics: List<MetricEntity>,
    val answeredMetricIds: Set<Int>,
) {
    val allAnswered get() = metrics.isNotEmpty() && answeredMetricIds.containsAll(metrics.map { it.metricId })
}

@Composable
fun PhoneScreen(modifier: Modifier = Modifier, initialGroupId: Int = 0) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    val enabled by PhoneMode.integrationEnabled.collectAsState()
    var dataVersion by remember { mutableIntStateOf(ImportRepository.dataVersion.value) }
    var passes by remember { mutableStateOf<List<DuePass>>(emptyList()) }
    var answering by remember { mutableStateOf<GroupEntity?>(null) }

    LaunchedEffect(Unit) { ImportRepository.dataVersion.collect { dataVersion = it } }

    LaunchedEffect(dataVersion, enabled) {
        passes = withContext(Dispatchers.IO) { loadDuePasses(context) }
        // Deep link from a reminder notification: jump straight into that pass.
        if (initialGroupId != 0 && answering == null) {
            answering = passes.firstOrNull { it.group.groupId == initialGroupId }?.group
        }
    }

    val current = answering
    if (current != null) {
        val pass = passes.firstOrNull { it.group.groupId == current.groupId }
        if (pass != null) {
            AnswerFlow(
                pass = pass,
                onAnswer = { metric, value ->
                    scope.launch(Dispatchers.IO) {
                        ConfigSync.saveRegistration(
                            context = context,
                            metricId = metric.metricId,
                            groupId = pass.group.groupId,
                            groupName = pass.group.name,
                            value = value,
                            timestamp = System.currentTimeMillis() / 1000,
                        )
                    }
                },
                onDone = { answering = null },
            )
            return
        }
        answering = null
    }

    Column(modifier = modifier.verticalScroll(rememberScrollState()).padding(16.dp)) {
        Card(Modifier.fillMaxWidth()) {
            Column(Modifier.padding(16.dp)) {
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Text("Pebble-integration", style = MaterialTheme.typography.titleMedium, modifier = Modifier.weight(1f))
                    Switch(checked = enabled, onCheckedChange = { on ->
                        scope.launch(Dispatchers.IO) { PhoneMode.setEnabled(context, on) }
                    })
                }
                Spacer(Modifier.height(8.dp))
                Text(
                    if (enabled)
                        "På: klockan sköter påminnelser och inmatning som vanligt."
                    else
                        "Av (telefonläge): klockans larm är avstängda. Telefonen påminner vid passtiderna och du svarar här. Svaren synkas tillbaka till klockan.",
                    style = MaterialTheme.typography.bodyMedium,
                )
            }
        }

        Spacer(Modifier.height(16.dp))

        if (!enabled) {
            Text("Dagens pass", style = MaterialTheme.typography.titleMedium)
            Spacer(Modifier.height(8.dp))
            if (passes.isEmpty()) {
                Text("Inga aktiva pass. Lägg till i Konfig.", style = MaterialTheme.typography.bodyMedium)
            }
            passes.forEach { pass ->
                Card(Modifier.fillMaxWidth().padding(vertical = 4.dp)) {
                    Row(
                        Modifier.padding(16.dp),
                        verticalAlignment = Alignment.CenterVertically,
                    ) {
                        Column(Modifier.weight(1f)) {
                            Text(
                                "${pass.group.name} · ${"%02d:%02d".format(pass.group.hour, pass.group.minute)}",
                                fontWeight = FontWeight.Bold,
                            )
                            Text(
                                if (pass.allAnswered) "Klart idag ✓"
                                else "${pass.answeredMetricIds.size}/${pass.metrics.size} besvarade",
                                style = MaterialTheme.typography.bodySmall,
                            )
                        }
                        Button(onClick = { answering = pass.group }) {
                            Text(if (pass.allAnswered) "Ändra" else "Svara")
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun AnswerFlow(
    pass: DuePass,
    onAnswer: (MetricEntity, Int) -> Unit,
    onDone: () -> Unit,
) {
    var index by remember(pass.group.groupId) { mutableIntStateOf(0) }

    if (index >= pass.metrics.size) {
        Column(Modifier.fillMaxWidth().padding(24.dp), horizontalAlignment = Alignment.CenterHorizontally) {
            Text("Klart!", style = MaterialTheme.typography.headlineSmall)
            Spacer(Modifier.height(16.dp))
            Button(onClick = onDone) { Text("Tillbaka") }
        }
        return
    }

    val metric = pass.metrics[index]
    Column(Modifier.fillMaxWidth().padding(24.dp)) {
        Text(
            "${pass.group.name} · ${index + 1}/${pass.metrics.size}",
            style = MaterialTheme.typography.labelMedium,
        )
        Spacer(Modifier.height(8.dp))
        Text(metric.name, style = MaterialTheme.typography.headlineSmall)
        Spacer(Modifier.height(24.dp))

        AnswerButtons(metric) { value ->
            onAnswer(metric, value)
            index++
        }

        Spacer(Modifier.height(24.dp))
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            TextButton(onClick = { index++ }) { Text("Hoppa över") }
            TextButton(onClick = onDone) { Text("Avbryt") }
        }
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
private fun AnswerButtons(metric: MetricEntity, onValue: (Int) -> Unit) {
    val labels = metric.optionTexts.split(UNIT_SEP).filter { it.isNotEmpty() }
    when (metric.type) {
        "bool" -> Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            OutlinedButton(onClick = { onValue(0) }) { Text(labels.getOrNull(0) ?: "Nej") }
            Button(onClick = { onValue(1) }) { Text(labels.getOrNull(1) ?: "Ja") }
        }
        "three_option" -> Row(horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            (0..2).forEach { v ->
                OutlinedButton(onClick = { onValue(v) }) { Text(labels.getOrNull(v) ?: "${v + 1}") }
            }
        }
        else -> FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp)) {  // interval
            val lo = metric.min
            val hi = if (metric.max > metric.min) metric.max else metric.min + 1
            (lo..hi).forEach { v ->
                OutlinedButton(onClick = { onValue(v) }) { Text("$v") }
            }
        }
    }
}

/** Loads active groups + their metrics + which are answered today. */
private suspend fun loadDuePasses(context: android.content.Context): List<DuePass> {
    val db = AppDatabase.get(context)
    val groups = db.groups().active()
    if (groups.isEmpty()) return emptyList()
    val allMetrics = db.metrics().all().associateBy { it.metricId }
    val dayStart = startOfTodaySeconds()
    return groups.map { g ->
        val memberIds = db.groups().membersOf(g.groupId)
        val metrics = memberIds.mapNotNull { allMetrics[it] }
        val answered = db.registrations()
            .inWindow(memberIds, dayStart)
            .filter { it.groupId == g.groupId }
            .map { it.metricId }
            .toSet()
        DuePass(g, metrics, answered)
    }
}

private fun startOfTodaySeconds(): Long {
    val c = Calendar.getInstance().apply {
        set(Calendar.HOUR_OF_DAY, 0); set(Calendar.MINUTE, 0)
        set(Calendar.SECOND, 0); set(Calendar.MILLISECOND, 0)
    }
    return c.timeInMillis / 1000
}
