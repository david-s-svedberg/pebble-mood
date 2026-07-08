package se.svep.mood.companion.health

import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.height
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.health.connect.client.HealthConnectClient
import androidx.activity.compose.rememberLauncherForActivityResult
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * Health Connect controls on the Status tab: availability, the permission
 * grant, and a manual "fetch" that pulls steps/sleep/heart-rate into the graph
 * + insights. Kept a manual pull for now (no background worker) — the data is
 * read-only history, so on-demand is enough.
 */
@Composable
fun HealthSection(modifier: Modifier = Modifier) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val manager = remember { HealthConnectManager(context) }

    var status by remember { mutableStateOf("") }
    var busy by remember { mutableStateOf(false) }

    fun runImport() {
        scope.launch {
            busy = true
            status = "Hämtar…"
            status = try {
                val r = withContext(Dispatchers.IO) { manager.importToRoom(30) }
                if (r.total == 0) "Inga hälsodata hittades (skriver Core Devices-appen till Health Connect?)."
                else "Hämtat: ${r.steps} dagar steg, ${r.sleep} sömn, ${r.heart} puls."
            } catch (e: Exception) {
                "Fel vid hämtning: ${e.message}"
            }
            busy = false
        }
    }

    val launcher = rememberLauncherForActivityResult(manager.requestPermissionsContract()) { granted ->
        if (granted.containsAll(manager.permissions)) runImport()
        else status = "Behörighet nekad."
    }

    LaunchedEffect(Unit) {
        status = when (manager.sdkStatus()) {
            HealthConnectClient.SDK_AVAILABLE ->
                if (manager.hasPermissions()) "Kopplad. Tryck för att uppdatera." else "Tryck för att koppla Health Connect."
            HealthConnectClient.SDK_UNAVAILABLE_PROVIDER_UPDATE_REQUIRED ->
                "Health Connect behöver uppdateras."
            else -> "Health Connect saknas på den här enheten."
        }
    }

    Column(modifier) {
        Text("Hälsodata (Health Connect)", style = MaterialTheme.typography.titleMedium)
        Spacer(Modifier.height(4.dp))
        Text(status, style = MaterialTheme.typography.bodyMedium, fontSize = 14.sp)
        Spacer(Modifier.height(8.dp))
        Button(
            enabled = !busy && manager.isAvailable(),
            onClick = {
                scope.launch {
                    if (manager.hasPermissions()) runImport()
                    else launcher.launch(manager.permissions)
                }
            },
        ) {
            if (busy) {
                CircularProgressIndicator(Modifier.height(18.dp), strokeWidth = 2.dp)
            } else {
                Text("Hämta hälsodata")
            }
        }
    }
}
