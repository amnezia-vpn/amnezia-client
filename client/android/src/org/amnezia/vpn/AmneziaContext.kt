package org.amnezia.vpn

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import androidx.core.content.ContextCompat
import androidx.core.content.ContextCompat.RegisterReceiverFlags
import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.CONNECTING
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTING
import org.amnezia.vpn.protocol.ProtocolState.RECONNECTING
import org.amnezia.vpn.protocol.ProtocolState.UNKNOWN

fun Context.getString(state: ProtocolState): String =
    getString(
        when (state) {
            DISCONNECTED, UNKNOWN -> R.string.disconnected
            CONNECTED -> R.string.connected
            CONNECTING -> R.string.connecting
            DISCONNECTING -> R.string.disconnecting
            RECONNECTING -> R.string.reconnecting
        }
    )

fun Context.registerBroadcastReceiver(
    action: String,
    @RegisterReceiverFlags flags: Int = ContextCompat.RECEIVER_EXPORTED,
    onReceive: () -> Unit
): BroadcastReceiver =
    object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            onReceive()
        }
    }.also {
        ContextCompat.registerReceiver(this, it, IntentFilter(action), flags)
    }

fun Context.unregisterBroadcastReceiver(receiver: BroadcastReceiver?) {
    receiver?.let { this.unregisterReceiver(it) }
}
