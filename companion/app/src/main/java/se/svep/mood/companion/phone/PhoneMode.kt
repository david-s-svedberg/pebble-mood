package se.svep.mood.companion.phone

import android.content.Context
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import se.svep.mood.companion.config.ConfigSync

/**
 * The global "Pebble-integration" setting (ROADMAP fas 5).
 *
 * - ON (default): the watch owns reminders + input, exactly as before.
 * - OFF (phone mode): the phone posts the reminders and takes the answers; the
 *   watch's group alarms are suspended (synced over) so it stays quiet.
 *
 * The switch is the single source of truth and lives in SharedPreferences —
 * it survives a watch reinstall (the watch has no matching persistent notion of
 * "who owns reminders"; it only knows the suspend flag we push it).
 */
object PhoneMode {

    private const val PREFS = "phone_mode"
    private const val KEY_INTEGRATION = "pebble_integration"

    private val enabledFlow = MutableStateFlow(true)
    /** true = Pebble integration on (watch owns reminders); false = phone mode. */
    val integrationEnabled = enabledFlow.asStateFlow()

    private fun prefs(context: Context) =
        context.applicationContext.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    /** Load the persisted value into the flow. Call once on app/service start. */
    fun load(context: Context) {
        enabledFlow.value = prefs(context).getBoolean(KEY_INTEGRATION, true)
    }

    fun isEnabled(context: Context): Boolean =
        prefs(context).getBoolean(KEY_INTEGRATION, true)

    /**
     * Flip the mode: persist it, queue the watch alarm-suspend sync, and either
     * arm the phone's local reminders or cancel them.
     */
    suspend fun setEnabled(context: Context, enabled: Boolean) {
        prefs(context).edit().putBoolean(KEY_INTEGRATION, enabled).apply()
        enabledFlow.value = enabled

        // Integration ON -> alarms live on the watch (not suspended).
        ConfigSync.saveAppMode(context, suspended = !enabled)

        if (enabled) {
            Reminders.cancelAll(context)
        } else {
            Reminders.rescheduleAll(context)
        }
    }
}
