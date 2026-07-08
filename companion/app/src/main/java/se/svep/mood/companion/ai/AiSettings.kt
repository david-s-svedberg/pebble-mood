package se.svep.mood.companion.ai

import android.content.Context
import android.content.SharedPreferences
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey

/**
 * Local settings for the optional AI analysis (phase 7). The API key and the
 * one-time consent flag live in an EncryptedSharedPreferences file (AES-256,
 * key in the Android Keystore) so the key isn't stored in plaintext on disk.
 * It's only ever sent to Anthropic's API on an explicit "Analysera period" tap.
 */
object AiSettings {

    // New (encrypted) store name — distinct from the old plaintext "ai" file so
    // opening it never tries to decrypt unencrypted data.
    private const val PREFS = "ai_secure"
    private const val KEY_API = "anthropic_api_key"
    private const val KEY_CONSENT = "consent_given"

    /** The Claude model used for the analysis. */
    const val MODEL = "claude-sonnet-5"

    @Volatile private var cached: SharedPreferences? = null

    private fun prefs(context: Context): SharedPreferences {
        cached?.let { return it }
        return synchronized(this) {
            cached ?: run {
                val appCtx = context.applicationContext
                val masterKey = MasterKey.Builder(appCtx)
                    .setKeyScheme(MasterKey.KeyScheme.AES256_GCM)
                    .build()
                EncryptedSharedPreferences.create(
                    appCtx,
                    PREFS,
                    masterKey,
                    EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
                    EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM,
                ).also { cached = it }
            }
        }
    }

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
