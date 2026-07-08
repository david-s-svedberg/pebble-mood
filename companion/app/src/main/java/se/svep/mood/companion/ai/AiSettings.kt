package se.svep.mood.companion.ai

import android.content.Context

/**
 * Local settings for the optional AI analysis (phase 7). The API key and the
 * one-time consent flag live in SharedPreferences on the phone; the key is only
 * ever sent to Anthropic's API on an explicit "Analysera period" tap.
 */
object AiSettings {

    private const val PREFS = "ai"
    private const val KEY_API = "anthropic_api_key"
    private const val KEY_CONSENT = "consent_given"

    /** The Claude model used for the analysis. */
    const val MODEL = "claude-sonnet-5"

    private fun prefs(context: Context) =
        context.applicationContext.getSharedPreferences(PREFS, Context.MODE_PRIVATE)

    fun apiKey(context: Context): String = prefs(context).getString(KEY_API, "") ?: ""

    fun hasApiKey(context: Context): Boolean = apiKey(context).isNotBlank()

    fun setApiKey(context: Context, key: String) {
        prefs(context).edit().putString(KEY_API, key.trim()).apply()
    }

    fun consentGiven(context: Context): Boolean = prefs(context).getBoolean(KEY_CONSENT, false)

    fun setConsent(context: Context, given: Boolean) {
        prefs(context).edit().putBoolean(KEY_CONSENT, given).apply()
    }
}
