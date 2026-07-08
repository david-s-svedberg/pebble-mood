package se.svep.mood.companion.phone

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import androidx.core.app.NotificationCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import se.svep.mood.companion.MainActivity
import se.svep.mood.companion.R
import se.svep.mood.companion.db.AppDatabase

/**
 * Fires at a group's scheduled time in phone mode: posts a reminder notification
 * (tap deep-links to that group's answer flow) and re-arms the alarm for the
 * next day. Skips the notification if the group is already fully answered today.
 */
class ReminderReceiver : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent) {
        val groupId = intent.getIntExtra(Reminders.EXTRA_GROUP_ID, 0)
        if (groupId == 0) return

        val pending = goAsync()
        CoroutineScope(Dispatchers.IO).launch {
            try {
                val db = AppDatabase.get(context)
                val group = db.groups().byId(groupId)
                if (group != null && group.active) {
                    // Re-arm for tomorrow (inexact daily) before anything else.
                    Reminders.armGroup(context, groupId, group.hour, group.minute)

                    if (!answeredToday(context, groupId)) {
                        notify(context, groupId, group.name)
                    }
                }
            } finally {
                pending.finish()
            }
        }
    }

    private suspend fun answeredToday(context: Context, groupId: Int): Boolean {
        val db = AppDatabase.get(context)
        val members = db.groups().membersOf(groupId).toSet()
        if (members.isEmpty()) return true
        val dayStart = startOfTodaySeconds()
        val answered = db.registrations()
            .inWindow(members.toList(), dayStart)
            .filter { it.groupId == groupId }
            .map { it.metricId }
            .toSet()
        return answered.containsAll(members)
    }

    private fun notify(context: Context, groupId: Int, groupName: String) {
        val manager = context.getSystemService(NotificationManager::class.java)
        manager.createNotificationChannel(
            NotificationChannel(CHANNEL, "Påminnelser", NotificationManager.IMPORTANCE_HIGH)
        )
        val tap = Intent(context, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP
            putExtra(Reminders.EXTRA_GROUP_ID, groupId)
        }
        val tapIntent = PendingIntent.getActivity(
            context, groupId, tap,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE,
        )
        val notification = NotificationCompat.Builder(context, CHANNEL)
            .setContentTitle("Dags att svara: $groupName")
            .setContentText("Tryck för att registrera")
            .setSmallIcon(R.drawable.ic_stat_mood)
            .setAutoCancel(true)
            .setContentIntent(tapIntent)
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .build()
        manager.notify(groupId, notification)
    }

    private fun startOfTodaySeconds(): Long {
        val c = java.util.Calendar.getInstance().apply {
            set(java.util.Calendar.HOUR_OF_DAY, 0)
            set(java.util.Calendar.MINUTE, 0)
            set(java.util.Calendar.SECOND, 0)
            set(java.util.Calendar.MILLISECOND, 0)
        }
        return c.timeInMillis / 1000
    }

    private companion object {
        const val CHANNEL = "reminders"
    }
}
