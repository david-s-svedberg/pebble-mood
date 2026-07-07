package se.svep.mood.companion.config

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject
import se.svep.mood.companion.ImportRepository
import se.svep.mood.companion.db.AppDatabase
import se.svep.mood.companion.db.GroupEntity
import se.svep.mood.companion.db.MembershipEntity
import se.svep.mood.companion.db.MetricEntity
import se.svep.mood.companion.db.PendingChangeEntity

/**
 * The phone->watch half of config sync: edits are applied to the local mirror
 * immediately (optimistic) and queued as pending changes; pkjs pulls the queue
 * when the watch app runs, applies each over AppMessage, then acks. Created
 * entities are NOT mirrored locally — the watch allocates their ids and the
 * following config export brings them in (no id mapping anywhere).
 */
object ConfigSync {

    /** MetricsType codes on the watch (data.h). */
    val TYPE_CODES = mapOf("bool" to 1, "interval" to 2, "three_option" to 3)

    suspend fun saveGroup(
        context: Context,
        groupId: Int,          // 0 = create on watch
        name: String,
        hour: Int,
        minute: Int,
        active: Boolean,
        memberIds: List<Int>,
    ) {
        val db = AppDatabase.get(context)
        if (groupId != 0) {
            db.groups().upsertAll(listOf(GroupEntity(groupId, name, hour, minute, active)))
            db.groups().clearMembers(groupId)
            db.groups().addMembers(memberIds.map { MembershipEntity(groupId, it) })
        }
        val payload = JSONObject()
            .put("kind", "group")
            .put("groupId", groupId)
            .put("name", name)
            .put("hour", hour)
            .put("minute", minute)
            .put("active", active)
            .put("members", JSONArray(memberIds))
        db.pending().enqueue(
            PendingChangeEntity(kind = "group", payload = payload.toString(), createdAt = System.currentTimeMillis())
        )
        ImportRepository.bumpDataVersion()
    }

    suspend fun saveMetric(
        context: Context,
        metricId: Int,         // 0 = create on watch
        name: String,
        type: String,          // wire name; only used at create
        min: Int,
        max: Int,
        mainIcon: Int,
    ) {
        val db = AppDatabase.get(context)
        if (metricId != 0) {
            val existing = db.metrics().byId(metricId) ?: return
            db.metrics().upsertAll(
                listOf(existing.copy(name = name, min = min, max = max, mainIcon = mainIcon))
            )
        }
        val payload = JSONObject()
            .put("kind", "metric")
            .put("metricId", metricId)
            .put("name", name)
            .put("typeCode", TYPE_CODES[type] ?: 0)
            .put("min", min)
            .put("max", max)
            .put("mainIcon", mainIcon)
        db.pending().enqueue(
            PendingChangeEntity(kind = "metric", payload = payload.toString(), createdAt = System.currentTimeMillis())
        )
        ImportRepository.bumpDataVersion()
    }

    /** GET /pending response: the queue in pkjs wire shape. */
    suspend fun pendingJson(context: Context): String {
        val changes = AppDatabase.get(context).pending().all()
        val array = JSONArray()
        changes.forEach { change ->
            array.put(JSONObject(change.payload).put("id", change.id))
        }
        return JSONObject().put("changes", array).toString()
    }

    /** POST /pending-ack: pkjs confirmed these landed on the watch. */
    suspend fun ack(context: Context, body: String): String {
        val ids = JSONObject(body).optJSONArray("ids") ?: JSONArray()
        val removed = AppDatabase.get(context).pending()
            .delete((0 until ids.length()).map { ids.getLong(it) })
        ImportRepository.bumpDataVersion()
        return JSONObject().put("status", "ok").put("removed", removed).toString()
    }

    suspend fun pendingCount(context: Context): Int =
        AppDatabase.get(context).pending().all().size

    /** Human summary for the Konfig tab's pending list. */
    fun describe(change: PendingChangeEntity): String {
        val payload = JSONObject(change.payload)
        val name = payload.optString("name", "?")
        val created = (payload.optInt("groupId", -1) == 0 || payload.optInt("metricId", -1) == 0)
        return when (change.kind) {
            "group" -> if (created) "Nytt pass: $name" else "Ändrat pass: $name"
            else -> if (created) "Ny metric: $name" else "Ändrad metric: $name"
        }
    }
}

/** IconChoice names in watch-enum order (icons.h — APPEND-ONLY over there). */
val ICON_CHOICE_NAMES = listOf(
    "Ingen", "Bock", "Kryss", "Upp", "Ner", "Humör", "Motion", "Piller",
    "Ledsen min", "Neutral min", "Glad min", "Nivå låg", "Nivå mitten", "Nivå hög",
    "Sol", "Måne", "Droppe", "Hjärta", "Blixt", "Kaffe", "Glas", "Termometer",
    "Telefon", "Moln", "Hantel", "Pratbubbla", "Kryssruta", "Äpple", "Måltavla", "Puls",
)
