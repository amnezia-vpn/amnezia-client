package org.amnezia.vpn.util

import android.app.Application
import android.content.Context
import android.content.SharedPreferences
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey
import kotlin.reflect.typeOf

private const val TAG = "Prefs"
private const val PREFS_FILE = "org.amnezia.vpn.prefs"
private const val SECURE_PREFS_FILE = "$PREFS_FILE.secure"

object Prefs {
    private lateinit var app: Application
    val prefs: SharedPreferences
        get() = try {
            EncryptedSharedPreferences(
                app,
                SECURE_PREFS_FILE,
                MasterKey(app)
            )
        } catch (e: Exception) {
            Log.e(TAG, "Getting Encryption Storage failed: $e, plaintext fallback")
            app.getSharedPreferences(PREFS_FILE, Context.MODE_PRIVATE)
        }

    fun init(app: Application) {
        Log.v(TAG, "Init Prefs")
        this.app = app
    }

    fun save(key: String, value: Boolean) =
        prefs.edit().putBoolean(key, value).apply()

    fun save(key: String, value: String?) =
        prefs.edit().putString(key, value).apply()

    fun save(key: String, value: Int) =
        prefs.edit().putInt(key, value).apply()

    fun save(key: String, value: Long) =
        prefs.edit().putLong(key, value).apply()

    fun save(key: String, value: Float) =
        prefs.edit().putFloat(key, value).apply()

    inline fun <reified T> load(key: String): T {
        return when (typeOf<T>()) {
            typeOf<Boolean>() -> prefs.getBoolean(key, false)
            typeOf<String>() -> prefs.getString(key, "")
            typeOf<Int>() -> prefs.getInt(key, 0)
            typeOf<Long>() -> prefs.getLong(key, 0L)
            typeOf<Float>() -> prefs.getFloat(key, 0f)
            else -> throw IllegalArgumentException("SharedPreferences does not support type: ${typeOf<T>()}")
        } as T
    }
}
