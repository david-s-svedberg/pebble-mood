package se.svep.mood.companion.ai

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.FilterChip
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.input.PasswordVisualTransformation
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * The optional AI-analysis controls (phase 7), shown atop the Insikter tab:
 * an API-key field, a period picker, and a "Analysera period" button that (after
 * one-time consent) sends the period's data to Claude and shows its reasoning.
 */
@Composable
fun AiAnalysisSection(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()

    var hasKey by remember { mutableStateOf(AiSettings.hasApiKey(context)) }
    var editingKey by remember { mutableStateOf(false) }
    var keyInput by remember { mutableStateOf("") }
    var days by remember { mutableIntStateOf(30) }
    var busy by remember { mutableStateOf(false) }
    var result by remember { mutableStateOf<String?>(null) }
    var error by remember { mutableStateOf<String?>(null) }
    var showConsent by remember { mutableStateOf(false) }

    fun runAnalysis() {
        scope.launch {
            busy = true; result = null; error = null
            when (val r = withContext(Dispatchers.IO) { AiAnalyzer.analyze(context, days) }) {
                is AiAnalyzer.Result.Ok -> result = r.text
                is AiAnalyzer.Result.Error -> error = r.message
            }
            busy = false
        }
    }

    if (showConsent) {
        AlertDialog(
            onDismissRequest = { showConsent = false },
            title = { Text("Skicka data till Claude?") },
            text = {
                Text(
                    "AI-analysen skickar den valda periodens dagsvärden (metrics + hälsodata) " +
                        "till Anthropics Claude-API för att hitta mönster. Det är enda gången data " +
                        "lämnar telefonen — allt annat stannar lokalt. Fortsätta?"
                )
            },
            confirmButton = {
                TextButton(onClick = {
                    AiSettings.setConsent(context, true)
                    showConsent = false
                    runAnalysis()
                }) { Text("Godkänn & analysera") }
            },
            dismissButton = { TextButton(onClick = { showConsent = false }) { Text("Avbryt") } },
        )
    }

    Card(modifier.fillMaxWidth()) {
        Column(Modifier.padding(16.dp)) {
            Text("AI-analys (valfritt)", style = MaterialTheme.typography.titleMedium)
            Spacer(Modifier.height(8.dp))

            if (!hasKey || editingKey) {
                Text(
                    "Klistra in en Anthropic API-nyckel. Den sparas bara på telefonen och " +
                        "används endast vid AI-analys.",
                    style = MaterialTheme.typography.bodySmall,
                )
                Spacer(Modifier.height(8.dp))
                OutlinedTextField(
                    value = keyInput,
                    onValueChange = { keyInput = it },
                    label = { Text("API-nyckel") },
                    singleLine = true,
                    visualTransformation = PasswordVisualTransformation(),
                    modifier = Modifier.fillMaxWidth(),
                )
                Spacer(Modifier.height(8.dp))
                Button(
                    enabled = keyInput.isNotBlank(),
                    onClick = {
                        AiSettings.setApiKey(context, keyInput)
                        keyInput = ""
                        hasKey = true
                        editingKey = false
                    },
                ) { Text("Spara nyckel") }
            } else {
                Row(verticalAlignment = androidx.compose.ui.Alignment.CenterVertically) {
                    Text("Period:", style = MaterialTheme.typography.bodyMedium)
                    Spacer(Modifier.width(8.dp))
                    listOf(14, 30, 90).forEach { d ->
                        FilterChip(
                            selected = days == d,
                            onClick = { days = d },
                            label = { Text("$d d") },
                            modifier = Modifier.padding(end = 6.dp),
                        )
                    }
                }
                Spacer(Modifier.height(8.dp))
                Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
                    Button(
                        enabled = !busy,
                        onClick = {
                            if (AiSettings.consentGiven(context)) runAnalysis() else showConsent = true
                        },
                    ) {
                        if (busy) CircularProgressIndicator(Modifier.height(18.dp), strokeWidth = 2.dp)
                        else Text("Analysera period")
                    }
                    TextButton(onClick = { editingKey = true }) { Text("Byt nyckel") }
                }
            }

            error?.let {
                Spacer(Modifier.height(8.dp))
                Text(it, color = MaterialTheme.colorScheme.error, style = MaterialTheme.typography.bodyMedium)
            }
            result?.let {
                Spacer(Modifier.height(12.dp))
                Text(it, style = MaterialTheme.typography.bodyMedium)
            }
        }
    }
}
