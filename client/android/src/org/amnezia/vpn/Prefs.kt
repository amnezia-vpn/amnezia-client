package org.amnezia.vpn

import android.content.Context
import android.content.SharedPreferences
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey

private const val TAG = "Prefs"
private const val PREFS_FILE = "org.amnezia.vpn.prefs"
private const val SECURE_PREFS_FILE = PREFS_FILE + ".secure"

object Prefs {
    fun get(context: Context, appContext: Context = context.applicationContext): SharedPreferences =
        try {
            EncryptedSharedPreferences(
                appContext,
                SECURE_PREFS_FILE,
                MasterKey(appContext)
            )
        } catch (e: Exception) {
            Log.e(TAG, "Getting Encryption Storage failed, plaintext fallback")
            appContext.getSharedPreferences(PREFS_FILE, Context.MODE_PRIVATE)
        }
}
