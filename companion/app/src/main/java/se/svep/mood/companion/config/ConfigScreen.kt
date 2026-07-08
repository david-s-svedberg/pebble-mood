package se.svep.mood.companion.config

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Checkbox
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.SegmentedButton
import androidx.compose.material3.SegmentedButtonDefaults
import androidx.compose.material3.SingleChoiceSegmentedButtonRow
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
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
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import se.svep.mood.companion.ImportRepository
import se.svep.mood.companion.health.HealthMetrics
import se.svep.mood.companion.db.AppDatabase
import se.svep.mood.companion.db.GroupEntity
import se.svep.mood.companion.db.MembershipEntity
import se.svep.mood.companion.db.MetricEntity
import se.svep.mood.companion.db.PendingChangeEntity

/**
 * Phase 4: the config editor. Groups and metrics are editable here; edits are
 * mirrored locally at once and queued for the watch — pkjs applies them the
 * next time the watch app runs, and the following config export confirms the
 * round trip. New entities get their ids from the watch and appear after the
 * next sync (listed under "Väntar på klockan" until then).
 */
@Composable
fun ConfigScreen(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    var metrics by remember { mutableStateOf<List<MetricEntity>>(emptyList()) }
    var groups by remember { mutableStateOf<List<GroupEntity>>(emptyList()) }
    var memberships by remember { mutableStateOf<List<MembershipEntity>>(emptyList()) }
    var pending by remember { mutableStateOf<List<PendingChangeEntity>>(emptyList()) }
    var dataVersion by remember { mutableIntStateOf(ImportRepository.dataVersion.value) }

    var editGroup by remember { mutableStateOf<GroupEntity?>(null) }
    var creatingGroup by remember { mutableStateOf(false) }
    var editMetric by remember { mutableStateOf<MetricEntity?>(null) }
    var creatingMetric by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        ImportRepository.dataVersion.collect { dataVersion = it }
    }
    LaunchedEffect(dataVersion) {
        withContext(Dispatchers.IO) {
            val db = AppDatabase.get(context)
            metrics = db.metrics().all()
            groups = db.groups().all()
            memberships = db.groups().memberships()
            pending = db.pending().all()
        }
    }

    Column(modifier = modifier.verticalScroll(rememberScrollState()).padding(16.dp)) {
        if (pending.isNotEmpty()) {
            Text("Väntar på klockan (${pending.size})", style = MaterialTheme.typography.titleMedium)
            Text(
                "Appliceras nästa gång Mood körs på klockan.",
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
            pending.forEach {
                Text("• ${ConfigSync.describe(it)}", style = MaterialTheme.typography.bodyMedium)
            }
            Spacer(Modifier.height(12.dp))
            HorizontalDivider()
            Spacer(Modifier.height(12.dp))
        }

        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("Pass", style = MaterialTheme.typography.titleMedium)
            Spacer(Modifier.width(12.dp))
            OutlinedButton(onClick = { creatingGroup = true }) { Text("Nytt pass") }
        }
        Spacer(Modifier.height(4.dp))
        if (groups.isEmpty()) {
            Text(
                "Ingen konfiguration synkad ännu — öppna Mood på klockan.",
                style = MaterialTheme.typography.bodyMedium,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
            )
        }
        groups.forEach { group ->
            val memberNames = memberships
                .filter { it.groupId == group.groupId }
                .mapNotNull { m -> metrics.find { it.metricId == m.metricId }?.name }
            Column(
                Modifier
                    .fillMaxWidth()
                    .clickable { editGroup = group }
                    .padding(vertical = 6.dp)
            ) {
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

        Row(verticalAlignment = Alignment.CenterVertically) {
            Text("Metrics", style = MaterialTheme.typography.titleMedium)
            Spacer(Modifier.width(12.dp))
            OutlinedButton(onClick = { creatingMetric = true }) { Text("Ny metric") }
        }
        Text(
            "Tryck på en metric för att redigera. Valens (bra/dåligt vid högt värde) " +
                "används för insikterna och sparas bara här.",
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
                Text(
                    "${metric.name}  ·  $scale  ·  ${ICON_CHOICE_NAMES.getOrElse(metric.mainIcon) { "?" }}",
                    fontWeight = FontWeight.SemiBold,
                    modifier = Modifier.fillMaxWidth().clickable { editMetric = metric },
                )
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

    if (editGroup != null || creatingGroup) {
        val group = editGroup
        GroupEditorDialog(
            group = group,
            allMetrics = metrics,
            initialMembers = if (group != null)
                memberships.filter { it.groupId == group.groupId }.map { it.metricId }.toSet()
            else emptySet(),
            onDismiss = { editGroup = null; creatingGroup = false },
            onSave = { name, hour, minute, active, members ->
                scope.launch(Dispatchers.IO) {
                    ConfigSync.saveGroup(context, group?.groupId ?: 0, name, hour, minute, active, members.toList())
                }
                editGroup = null
                creatingGroup = false
            },
            onDelete = group?.let {
                {
                    scope.launch(Dispatchers.IO) { ConfigSync.deleteGroup(context, it.groupId) }
                    editGroup = null
                }
            },
        )
    }

    if (editMetric != null || creatingMetric) {
        val metric = editMetric
        MetricEditorDialog(
            metric = metric,
            onDismiss = { editMetric = null; creatingMetric = false },
            onSave = { name, type, min, max, mainIcon ->
                scope.launch(Dispatchers.IO) {
                    ConfigSync.saveMetric(context, metric?.metricId ?: 0, name, type, min, max, mainIcon)
                }
                editMetric = null
                creatingMetric = false
            },
            onDelete = metric?.let {
                {
                    scope.launch(Dispatchers.IO) { ConfigSync.deleteMetric(context, it.metricId) }
                    editMetric = null
                }
            },
        )
    }
}

@Composable
private fun GroupEditorDialog(
    group: GroupEntity?,
    allMetrics: List<MetricEntity>,
    initialMembers: Set<Int>,
    onDismiss: () -> Unit,
    onSave: (name: String, hour: Int, minute: Int, active: Boolean, members: Set<Int>) -> Unit,
    onDelete: (() -> Unit)? = null,
) {
    var name by remember { mutableStateOf(group?.name ?: "") }
    var hourText by remember { mutableStateOf((group?.hour ?: 8).toString()) }
    var minuteText by remember { mutableStateOf((group?.minute ?: 0).toString()) }
    var active by remember { mutableStateOf(group?.active ?: true) }
    var members by remember { mutableStateOf(initialMembers) }
    var confirmDelete by remember { mutableStateOf(false) }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(if (group == null) "Nytt pass" else "Redigera pass") },
        text = {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                OutlinedTextField(value = name, onValueChange = { name = it }, label = { Text("Namn") })
                Spacer(Modifier.height(8.dp))
                Row {
                    OutlinedTextField(
                        value = hourText, onValueChange = { hourText = it.filter(Char::isDigit).take(2) },
                        label = { Text("Timme") }, modifier = Modifier.width(100.dp),
                    )
                    Spacer(Modifier.width(8.dp))
                    OutlinedTextField(
                        value = minuteText, onValueChange = { minuteText = it.filter(Char::isDigit).take(2) },
                        label = { Text("Minut") }, modifier = Modifier.width(100.dp),
                    )
                }
                Spacer(Modifier.height(8.dp))
                Row(verticalAlignment = Alignment.CenterVertically) {
                    Switch(checked = active, onCheckedChange = { active = it })
                    Spacer(Modifier.width(8.dp))
                    Text("Alarm aktivt")
                }
                Spacer(Modifier.height(8.dp))
                Text("Metrics i passet:", style = MaterialTheme.typography.labelLarge)
                // Health Connect auto-metrics (companion-only ids) aren't watch
                // metrics — excluded so they can't be synced as group members
                // (SET_GROUP_MEMBERS is uint8; their high ids would truncate).
                allMetrics.filter { it.metricId !in HealthMetrics.ALL }.forEach { metric ->
                    Row(verticalAlignment = Alignment.CenterVertically) {
                        Checkbox(
                            checked = metric.metricId in members,
                            onCheckedChange = { checked ->
                                members = if (checked) members + metric.metricId else members - metric.metricId
                            },
                        )
                        Text(metric.name)
                    }
                }
                if (onDelete != null) {
                    Spacer(Modifier.height(8.dp))
                    TextButton(
                        onClick = { if (confirmDelete) onDelete() else confirmDelete = true },
                        colors = ButtonDefaults.textButtonColors(contentColor = MaterialTheme.colorScheme.error),
                    ) { Text(if (confirmDelete) "Tryck igen för att ta bort" else "Ta bort pass") }
                }
            }
        },
        confirmButton = {
            TextButton(
                enabled = name.isNotBlank(),
                onClick = {
                    onSave(
                        name.trim(),
                        (hourText.toIntOrNull() ?: 8).coerceIn(0, 23),
                        (minuteText.toIntOrNull() ?: 0).coerceIn(0, 59),
                        active,
                        members,
                    )
                },
            ) { Text("Spara") }
        },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Avbryt") } },
    )
}

@Composable
private fun MetricEditorDialog(
    metric: MetricEntity?,
    onDismiss: () -> Unit,
    onSave: (name: String, type: String, min: Int, max: Int, mainIcon: Int) -> Unit,
    onDelete: (() -> Unit)? = null,
) {
    var name by remember { mutableStateOf(metric?.name ?: "") }
    var type by remember { mutableStateOf(metric?.type ?: "interval") }
    var minText by remember { mutableStateOf((metric?.min ?: 0).toString()) }
    var maxText by remember { mutableStateOf((metric?.max ?: 5).toString()) }
    var mainIcon by remember { mutableStateOf(metric?.mainIcon ?: 0) }
    var iconMenuOpen by remember { mutableStateOf(false) }
    var confirmDelete by remember { mutableStateOf(false) }

    AlertDialog(
        onDismissRequest = onDismiss,
        title = { Text(if (metric == null) "Ny metric" else "Redigera metric") },
        text = {
            Column(Modifier.verticalScroll(rememberScrollState())) {
                OutlinedTextField(value = name, onValueChange = { name = it }, label = { Text("Namn") })
                Spacer(Modifier.height(8.dp))

                if (metric == null) {
                    Text("Typ:", style = MaterialTheme.typography.labelLarge)
                    SingleChoiceSegmentedButtonRow(Modifier.fillMaxWidth()) {
                        listOf("bool" to "Ja/nej", "interval" to "Intervall", "three_option" to "3 alt")
                            .forEachIndexed { i, (value, label) ->
                                SegmentedButton(
                                    selected = type == value,
                                    onClick = { type = value },
                                    shape = SegmentedButtonDefaults.itemShape(i, 3),
                                ) { Text(label, fontSize = 12.sp) }
                            }
                    }
                    Spacer(Modifier.height(8.dp))
                }

                if (type == "interval") {
                    Row {
                        OutlinedTextField(
                            value = minText, onValueChange = { minText = it.filter(Char::isDigit).take(1) },
                            label = { Text("Min (0/1)") }, modifier = Modifier.width(110.dp),
                        )
                        Spacer(Modifier.width(8.dp))
                        OutlinedTextField(
                            value = maxText, onValueChange = { maxText = it.filter(Char::isDigit).take(2) },
                            label = { Text("Max") }, modifier = Modifier.width(100.dp),
                        )
                    }
                    Spacer(Modifier.height(8.dp))
                }

                Text("Huvudikon:", style = MaterialTheme.typography.labelLarge)
                OutlinedButton(onClick = { iconMenuOpen = true }) {
                    Text(ICON_CHOICE_NAMES.getOrElse(mainIcon) { "?" })
                }
                DropdownMenu(expanded = iconMenuOpen, onDismissRequest = { iconMenuOpen = false }) {
                    ICON_CHOICE_NAMES.forEachIndexed { index, iconName ->
                        DropdownMenuItem(
                            text = { Text(iconName) },
                            onClick = { mainIcon = index; iconMenuOpen = false },
                        )
                    }
                }
                if (onDelete != null) {
                    Spacer(Modifier.height(8.dp))
                    TextButton(
                        onClick = { if (confirmDelete) onDelete() else confirmDelete = true },
                        colors = ButtonDefaults.textButtonColors(contentColor = MaterialTheme.colorScheme.error),
                    ) { Text(if (confirmDelete) "Tryck igen för att ta bort" else "Ta bort metric") }
                }
            }
        },
        confirmButton = {
            TextButton(
                enabled = name.isNotBlank(),
                onClick = {
                    val min = (minText.toIntOrNull() ?: 0).coerceIn(0, 1)
                    val max = (maxText.toIntOrNull() ?: 5).coerceIn(min + 1, 10)
                    onSave(name.trim(), type, min, max, mainIcon)
                },
            ) { Text("Spara") }
        },
        dismissButton = { TextButton(onClick = onDismiss) { Text("Avbryt") } },
    )
}
