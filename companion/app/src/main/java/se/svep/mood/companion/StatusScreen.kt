package se.svep.mood.companion

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import se.svep.mood.companion.db.AppDatabase
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

@Composable
fun StatusScreen(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    var text by remember { mutableStateOf("Laddar…") }
    var lastImport by remember { mutableStateOf(ImportRepository.lastImport.value) }

    LaunchedEffect(Unit) {
        ImportRepository.lastImport.collect { lastImport = it }
    }

    LaunchedEffect(lastImport) {
        text = withContext(Dispatchers.IO) {
            val db = AppDatabase.get(context)
            val total = db.registrations().count()
            val newest = db.registrations().newestTimestamp()
            val perMetric = db.registrations().countsPerMetric()
            val time = SimpleDateFormat("yyyy-MM-dd HH:mm", Locale.ROOT)

            buildString {
                appendLine("Synk-tjänsten lyssnar på localhost:9099.")
                appendLine()
                lastImport?.let { (result, at) ->
                    appendLine("Senaste import ${time.format(Date(at))}: ${result.received} mottagna, ${result.new} nya.")
                } ?: appendLine("Ingen import den här sessionen ännu.")
                appendLine()
                appendLine("I databasen: $total registreringar")
                newest?.let { appendLine("Senaste svar: ${time.format(Date(it * 1000))}") }
                if (perMetric.isNotEmpty()) {
                    appendLine()
                    appendLine("Per metric:")
                    perMetric.forEach { appendLine("  ${it.name}: ${it.count}") }
                }
            }
        }
    }

    Column(modifier = modifier.verticalScroll(rememberScrollState()).padding(16.dp)) {
        Text(text, style = MaterialTheme.typography.bodyLarge)
    }
}
