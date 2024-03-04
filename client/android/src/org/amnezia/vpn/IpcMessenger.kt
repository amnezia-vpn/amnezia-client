package org.amnezia.vpn

import android.os.DeadObjectException
import android.os.Message
import android.os.Messenger
import android.os.RemoteException
import org.amnezia.vpn.util.Log

private const val TAG = "IpcMessenger"

class IpcMessenger(
    messengerName: String? = null,
    private val onDeadObjectException: () -> Unit = {},
    private val onRemoteException: () -> Unit = {}
) {
    private var messenger: Messenger? = null
    val name = messengerName ?: "Unknown"

    constructor(
        messenger: Messenger,
        messengerName: String? = null,
        onDeadObjectException: () -> Unit = {},
        onRemoteException: () -> Unit = {}
    ) : this(messengerName, onDeadObjectException, onRemoteException) {
        this.messenger = messenger
    }

    fun set(messenger: Messenger) {
        this.messenger = messenger
    }

    fun reset() {
        messenger = null
    }

    fun send(msg: () -> Message) = messenger?.sendMsg(msg())

    fun send(msg: Message, replyTo: Messenger) = messenger?.sendMsg(msg.apply { this.replyTo = replyTo })

    fun <T> send(msg: T)
        where T : Enum<T>, T : IpcMessage = messenger?.sendMsg(msg.packToMessage())

    fun <T> send(msg: T, replyTo: Messenger)
        where T : Enum<T>, T : IpcMessage = messenger?.sendMsg(msg.packToMessage().apply { this.replyTo = replyTo })

    private fun Messenger.sendMsg(msg: Message) {
        try {
            send(msg)
        } catch (e: DeadObjectException) {
            Log.w(TAG, "$name messenger is dead")
            messenger = null
            onDeadObjectException()
        } catch (e: RemoteException) {
            Log.w(TAG, "Sending a message to the $name messenger failed: ${e.message}")
            onRemoteException()
        }
    }
}

fun Map<Messenger, IpcMessenger>.send(msg: () -> Message) = this.values.forEach { it.send(msg) }

fun <T> Map<Messenger, IpcMessenger>.send(msg: T)
    where T : Enum<T>, T : IpcMessage = this.values.forEach { it.send(msg) }
