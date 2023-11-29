package org.amnezia.vpn

import android.os.Bundle
import android.os.Message
import kotlin.enums.enumEntries

sealed interface IpcMessage {
    companion object {
        @OptIn(ExperimentalStdlibApi::class)
        inline fun <reified T> extractFromMessage(msg: Message): T
            where T : Enum<T>,
                  T : IpcMessage {
            val values = enumEntries<T>()
            if (msg.what !in values.indices) {
                throw IllegalArgumentException("IPC action or event not found for the message: $msg")
            }
            return values[msg.what]
        }
    }
}

enum class ServiceEvent : IpcMessage {
    CONNECTED,
    DISCONNECTED,
    STATUS,
    STATISTICS_UPDATE,
    ERROR
}

enum class Action : IpcMessage {
    REGISTER_CLIENT,
    CONNECT,
    DISCONNECT,
    REQUEST_STATUS
}

fun <T> T.packToMessage(): Message
    where T : Enum<T>, T : IpcMessage = Message.obtain().also { it.what = ordinal }

fun <T> T.packToMessage(block: Bundle.() -> Unit): Message
    where T : Enum<T>, T : IpcMessage = packToMessage().also { it.data = Bundle().apply(block) }

inline fun <reified T> Message.extractIpcMessage(): T
    where T : Enum<T>, T : IpcMessage = IpcMessage.extractFromMessage<T>(this)
