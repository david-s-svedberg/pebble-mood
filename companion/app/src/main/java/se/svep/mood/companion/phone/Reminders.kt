package se.svep.mood.companion.phone

import android.app.AlarmManager
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import android.util.Log
import se.svep.mood.companion.db.AppDatabase
import java.util.Calendar

/**
 * Local daily reminders for phone mode — one AlarmManager alarm per active
 * group, firing at the group's time. We use setAndAllowWhileIdle (inexact) and
 * re-arm the next day when it fires: no SCHEDULE_EXACT_ALARM permission needed,
 * and a few minutes of doze slack is fine for a mood ping.
 *
 * The set of currently-armed group ids is tracked in prefs so we can cancel
 * them even after the group list has changed underneath us.
 */
object Reminders {

    private const val PREFS = "phone_reminders"
    private const val KEY_ARMED = "armed_group_ids"
    const val EXTRA_GROUP_ID = "groupId"
    private const val TAG = "MoodReminders"

    private fun prefs(context: Context) =
        context.applicationContext.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    private fun alarmManager(context: Context) =
        context.getSystemService(Context.ALARM_SERVICE) as AlarmManager

    private fun intent(context: Context, groupId: Int): PendingIntent {
        val i = Intent(context, ReminderReceiver::class.java).apply {
            action = "se.svep.mood.companion.REMIND"
            putExtra(EXTRA_GROUP_ID, groupId)
        }
        return PendingIntent.getBroadcast(
            context, groupId, i,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
        )
    }

    /** Next occurrence of hour:minute (today if still ahead, else tomorrow). */
    private fun nextTrigger(hour: Int, minute: Int): Long {
        val c = Calendar.getInstance().apply {
            set(Calendar.HOUR_OF_DAY, hour)
            set(Calendar.MINUTE, minute)
            set(Calendar.SECOND, 0)
            set(Calendar.MILLISECOND, 0)
        }
        if (c.timeInMillis <= System.currentTimeMillis()) {
            c.add(Calendar.DAY_OF_MONTH, 1)
        }
        return c.timeInMillis
    }

    /** Cancel then re-arm one alarm for the next day (called from the receiver). */
    fun armGroup(context: Context, groupId: Int, hour: Int, minute: Int) {
        val at = nextTrigger(hour, minute)
        alarmManager(context).setAndAllowWhileIdle(AlarmManager.RTC_WAKEUP, at, intent(context, groupId))
        Log.i(TAG, "armed group $groupId for $hour:$minute (at=$at)")
    }

    /** Re-arm every active group's reminder; forget any that are gone. */
    suspend fun rescheduleAll(context: Context) {
        cancelAll(context)
        val groups = AppDatabase.get(context).groups().active()
        val armed = HashSet<String>()
        groups.forEach { g ->
            armGroup(context, g.groupId, g.hour, g.minute)
            armed.add(g.groupId.toString())
        }
        prefs(context).edit().putStringSet(KEY_ARMED, armed).apply()
        Log.i(TAG, "rescheduled ${armed.size} reminders")
    }

    fun cancelAll(context: Context) {
        val armed = prefs(context).getStringSet(KEY_ARMED, emptySet()) ?: emptySet()
        armed.forEach { id ->
            id.toIntOrNull()?.let { alarmManager(context).cancel(intent(context, it)) }
        }
        prefs(context).edit().remove(KEY_ARMED).apply()
    }
}
