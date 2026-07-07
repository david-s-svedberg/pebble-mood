package se.svep.mood.companion.config

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.SegmentedButton
import androidx.compose.material3.SegmentedButtonDefaults
import androidx.compose.material3.SingleChoiceSegmentedButtonRow
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import se.svep.mood.companion.ImportRepository
import se.svep.mood.companion.db.AppDatabase
import se.svep.mood.companion.db.GroupEntity
import se.svep.mood.companion.db.MembershipEntity
import se.svep.mood.companion.db.MetricEntity

/**
 * Phase 4 stage A: read view of the watch-synced configuration (groups with
 * times and members, metric definitions) plus VALENCE editing — the one field
 * that is companion-only and doesn't need watch sync. Editing the rest (and
 * pushing changes back to the watch) is stage B.
 */
@Composable
fun ConfigScreen(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    var metrics by remember { mutableStateOf<List<MetricEntity>>(emptyList()) }
    var groups by remember { mutableStateOf<List<GroupEntity>>(emptyList()) }
    var memberships by remember { mutableStateOf<List<MembershipEntity>>(emptyList()) }
    var dataVersion by remember { mutableIntStateOf(ImportRepository.dataVersion.value) }

    LaunchedEffect(Unit) {
        ImportRepository.dataVersion.collect { dataVersion = it }
    }
    LaunchedEffect(dataVersion) {
        withContext(Dispatchers.IO) {
            val db = AppDatabase.get(context)
            metrics = db.metrics().all()
            groups = db.groups().all()
            memberships = db.groups().memberships()
        }
    }

    Column(modifier = modifier.verticalScroll(rememberScrollState()).padding(16.dp)) {
        Text("Pass", style = MaterialTheme.typography.titleMedium)
        Spacer(Modifier.height(8.dp))
        if (groups.isEmpty()) {
            Text(
                "Ingen konfiguration synkad ännu — öppna Mood på klockan så exporteras den.",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
        }
        groups.forEach { group ->
            val memberNames = memberships
                .filter { it.groupId == group.groupId }
                .mapNotNull { m -> metrics.find { it.metricId == m.metricId }?.name }
            Column(Modifier.padding(vertical = 6.dp)) {
                Text(
                    "%s  ·  %02d:%02d%s".format(
                        group.name, group.hour, group.minute,
                        if (group.active) "" else "  (inaktiv)",
                    ),
                    fontWeight = FontWeight.SemiBold,
                )
                Text(
                    if (memberNames.isEmpty()) "inga metrics" else memberNames.joinToString(", "),
                    style = MaterialTheme.typography.bodyMedium,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            }
        }

        Spacer(Modifier.height(16.dp))
        HorizontalDivider()
        Spacer(Modifier.height(16.dp))

        Text("Metrics", style = MaterialTheme.typography.titleMedium)
        Text(
            "Valens: är ett högt värde bra eller dåligt? Används för att rama in " +
                "insikterna — sparas bara i den här appen.",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
        )
        Spacer(Modifier.height(8.dp))

        metrics.forEach { metric ->
            Column(Modifier.padding(vertical = 6.dp)) {
                val scale = when (metric.type) {
                    "bool" -> "ja/nej"
                    "three_option" -> "3 alternativ"
                    else -> "${metric.min}–${metric.max}"
                }
                Text("${metric.name}  ·  $scale", fontWeight = FontWeight.SemiBold)
                Spacer(Modifier.height(4.dp))
                SingleChoiceSegmentedButtonRow(Modifier.fillMaxWidth()) {
                    listOf(-1 to "Negativ", 0 to "Neutral", 1 to "Positiv").forEachIndexed { i, (value, label) ->
                        SegmentedButton(
                            selected = metric.valence == value,
                            onClick = {
                                scope.launch(Dispatchers.IO) {
                                    AppDatabase.get(context).metrics().setValence(metric.metricId, value)
                                    ImportRepository.bumpDataVersion()
                                }
                            },
                            shape = SegmentedButtonDefaults.itemShape(i, 3),
                        ) { Text(label, fontSize = 12.sp) }
                    }
                }
            }
        }
    }
}
