package se.svep.mood.companion.insights

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import se.svep.mood.companion.ImportRepository
import se.svep.mood.companion.db.AppDatabase
import kotlin.math.abs

/**
 * Phase 7 v1: the built-in insight list. Correlations are presented with
 * direction language only — metric valence (is high good or bad?) is a
 * planned metadata field; once it exists the framing can say "seems good
 * for you" instead of just "moves together/opposite".
 */
@Composable
fun InsightsScreen(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    var insights by remember { mutableStateOf<List<Correlations.Insight>?>(null) }
    var dataVersion by remember { mutableIntStateOf(ImportRepository.dataVersion.value) }

    LaunchedEffect(Unit) {
        ImportRepository.dataVersion.collect { dataVersion = it }
    }

    LaunchedEffect(dataVersion) {
        insights = withContext(Dispatchers.IO) {
            val db = AppDatabase.get(context)
            val metrics = db.metrics().all()
            val from = System.currentTimeMillis() / 1000 - 90 * 86_400L
            val regs = db.registrations().inWindow(metrics.map { it.metricId }, from)
            Correlations.compute(metrics, regs)
        }
    }

    Column(modifier = modifier.verticalScroll(rememberScrollState()).padding(16.dp)) {
        Text("Samband (senaste 90 dagarna)", style = MaterialTheme.typography.titleMedium)
        Spacer(Modifier.height(4.dp))
        Text(
            "Spearman-korrelation på dagliga värden. Samband är inte orsak — " +
                "se det som ledtrådar värda att titta närmare på.",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
        )
        Spacer(Modifier.height(12.dp))

        val list = insights
        when {
            list == null -> Text("Beräknar…")
            list.isEmpty() -> Text(
                "Inga tydliga samband ännu — det behövs minst ~7 dagar där två " +
                    "metrics båda har svar. Fortsätt registrera (eller skapa " +
                    "demodata under Status för att se hur det kommer se ut)."
            )
            else -> list.forEach { insight ->
                InsightRow(insight)
                Spacer(Modifier.height(10.dp))
            }
        }
    }
}

@Composable
private fun InsightRow(insight: Correlations.Insight) {
    val strength = when {
        abs(insight.r) >= 0.6 -> "starkt"
        abs(insight.r) >= 0.4 -> "måttligt"
        else -> "svagt"
    }
    val direction = if (insight.r > 0) "åt samma håll" else "åt motsatt håll"
    val title = if (insight.lagDays == 0) {
        "${insight.a.name} ↔ ${insight.b.name}"
    } else {
        "${insight.a.name} → ${insight.b.name} dagen efter"
    }
    val caveat = if (insight.n < 14) " · få datapunkter" else ""

    // Valence framing (set per metric under Konfig): with both valences known,
    // the correlation's human meaning is computable — moving together is
    // favorable when the valences agree (r * valA * valB > 0), etc.
    val framing = if (insight.a.valence != 0 && insight.b.valence != 0 && abs(insight.r) >= 0.4) {
        if (insight.r * insight.a.valence * insight.b.valence > 0) " · verkar gynnsamt 👍"
        else " · verkar ogynnsamt 👎"
    } else ""

    Column {
        Text(title, style = MaterialTheme.typography.titleSmall)
        Text(
            "$strength samband $direction$framing",
            style = MaterialTheme.typography.bodyMedium,
        )
        Text(
            "r ${"%+.2f".format(insight.r)} · ${insight.n} dagar$caveat",
            style = MaterialTheme.typography.bodySmall,
            color = MaterialTheme.colorScheme.onSurfaceVariant,
        )
    }
}
