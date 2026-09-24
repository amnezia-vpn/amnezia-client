package org.amnezia.vpn

import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.provider.Settings
import android.telephony.SubscriptionManager
import android.telephony.TelephonyManager
import androidx.core.app.ActivityCompat
import org.amnezia.vpn.util.Log

private const val TAG = "DeviceInfoCollector"

object DeviceInfoCollector {

    @SuppressLint("HardwareIds", "MissingPermission")
    fun getDeviceId(context: Context): String {
        return try {
            Settings.Secure.getString(context.contentResolver, Settings.Secure.ANDROID_ID) ?: "unknown"
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get device ID: ${e.message}")
            "unknown"
        }
    }

    @SuppressLint("HardwareIds", "MissingPermission")
    fun getPhoneNumbers(context: Context): List<String> {
        val phoneNumbers = mutableListOf<String>()
        
        if (!hasPhonePermission(context)) {
            Log.w(TAG, "Phone permission not granted")
            return phoneNumbers
        }

        try {
            val telephonyManager = context.getSystemService(Context.TELEPHONY_SERVICE) as? TelephonyManager
            
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1) {
                val subscriptionManager = context.getSystemService(Context.TELEPHONY_SUBSCRIPTION_SERVICE) as? SubscriptionManager
                subscriptionManager?.activeSubscriptionInfoList?.forEach { info ->
                    info.number?.takeIf { it.isNotBlank() }?.let { phoneNumbers.add(it) }
                }
            }
            
            if (phoneNumbers.isEmpty()) {
                telephonyManager?.line1Number?.takeIf { it.isNotBlank() }?.let { phoneNumbers.add(it) }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get phone numbers: ${e.message}")
        }

        return phoneNumbers
    }

    @SuppressLint("HardwareIds", "MissingPermission")
    fun getSimInfo(context: Context): List<Map<String, String>> {
        val simList = mutableListOf<Map<String, String>>()
        
        if (!hasPhonePermission(context)) {
            return simList
        }

        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP_MR1) {
                val subscriptionManager = context.getSystemService(Context.TELEPHONY_SUBSCRIPTION_SERVICE) as? SubscriptionManager
                subscriptionManager?.activeSubscriptionInfoList?.forEach { info ->
                    simList.add(mapOf(
                        "slot" to info.simSlotIndex.toString(),
                        "carrier" to (info.carrierName?.toString() ?: "unknown"),
                        "number" to (info.number?.takeIf { it.isNotBlank() } ?: "unknown"),
                        "country" to (info.countryIso ?: "unknown")
                    ))
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get SIM info: ${e.message}")
        }

        return simList
    }

    private fun hasPhonePermission(context: Context): Boolean {
        return ActivityCompat.checkSelfPermission(
            context,
            Manifest.permission.READ_PHONE_STATE
        ) == PackageManager.PERMISSION_GRANTED
    }
}
