/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.amnezia.vpn

import android.content.Context
import android.content.SharedPreferences
import android.util.Log
import androidx.security.crypto.EncryptedSharedPreferences
import androidx.security.crypto.MasterKey

object Prefs {
    // Opens and returns an instance of EncryptedSharedPreferences
    fun get(context: Context): SharedPreferences {
        try {
            val mainKey = MasterKey.Builder(context.applicationContext)
                .setKeyScheme(MasterKey.KeyScheme.AES256_GCM)
                .build()

            val sharedPrefsFile = "com.amnezia.vpn.secure.prefs"
            val sharedPreferences: SharedPreferences = EncryptedSharedPreferences.create(
                context.applicationContext,
                sharedPrefsFile,
                mainKey,
                EncryptedSharedPreferences.PrefKeyEncryptionScheme.AES256_SIV,
                EncryptedSharedPreferences.PrefValueEncryptionScheme.AES256_GCM
            )
            return sharedPreferences
        } catch (e: Exception) {
            Log.e("Android-Prefs", "Getting Encryption Storage failed, plaintext fallback")
            return context.getSharedPreferences("com.amnezia.vpn.preferences", Context.MODE_PRIVATE)
        }
    }
}
