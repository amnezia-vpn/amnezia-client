package org.amnezia.vpn

import android.os.DeadObjectException
import android.os.Message
import android.os.Messenger
import android.os.RemoteException

private const val TAG = "IpcMessenger"

class IpcMessenger(
    private val onDeadObjectException: () -> Unit = {},
    private val onRemoteException: () -> Unit = {},
    private val messengerName: String = "Unknown"
) {
    private var messenger: Messenger? = null

    fun set(messenger: Messenger) {
        this.messenger = messenger
    }

    fun reset() {
        messenger = null
    }

    fun send(msg: () -> Message) = messenger?.sendMsg(msg())

    fun <T> send(msg: T)
        where T : Enum<T>, T : IpcMessage = messenger?.sendMsg(msg.packToMessage())

    private fun Messenger.sendMsg(msg: Message) {
        try {
            send(msg)
        } catch (e: DeadObjectException) {
            Log.w(TAG, "$messengerName messenger is dead")
            messenger = null
            onDeadObjectException()
        } catch (e: RemoteException) {
            Log.w(TAG, "Sending a message to the $messengerName messenger failed: ${e.message}")
            onRemoteException()
        }
    }
}
