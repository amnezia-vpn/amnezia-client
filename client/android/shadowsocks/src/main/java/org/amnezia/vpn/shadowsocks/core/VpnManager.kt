package org.amnezia.vpn.shadowsocks.core

import android.annotation.SuppressLint
import android.app.Activity
import android.content.Context
import android.content.Intent
import android.net.VpnService
import android.os.DeadObjectException
import android.os.Handler
import org.amnezia.vpn.shadowsocks.core.aidl.IShadowsocksService
import org.amnezia.vpn.shadowsocks.core.aidl.ShadowsocksConnection
import org.amnezia.vpn.shadowsocks.core.aidl.TrafficStats
import org.amnezia.vpn.shadowsocks.core.bg.BaseService
import org.amnezia.vpn.shadowsocks.core.preference.DataStore
import org.amnezia.vpn.shadowsocks.core.utils.Key

class VpnManager private constructor() {

     var state = BaseService.State.Idle
    private var context: Context? = null
    private val handler = Handler()
    private val connection = ShadowsocksConnection(handler, true)
    private var listener: OnStatusChangeListener? = null
    private val callback: ShadowsocksConnection.Callback = object : ShadowsocksConnection.Callback {
        override fun stateChanged(state: BaseService.State, profileName: String?, msg: String?) {
            changeState(state)
        }

        override fun onServiceDisconnected() = changeState(BaseService.State.Idle)

        override fun onServiceConnected(service: IShadowsocksService) {
            changeState(try {
                BaseService.State.values()[service.state]
            } catch (_: DeadObjectException) {
                BaseService.State.Idle
            })
        }

        override fun trafficUpdated(profileId: Long, stats: TrafficStats) {
            super.trafficUpdated(profileId, stats)
            listener?.onTrafficUpdated(profileId, stats)
        }
        override fun onBinderDied() {
            disconnect()
            connect()
        }
    }

    private fun connect() {
        context?.let {
            connection.connect(it, callback)
        }
    }

    private fun disconnect() {
        context?.let { connection.disconnect(it) }
    }

    companion object {
        private const val REQUEST_CONNECT = 1
        @SuppressLint("StaticFieldLeak")
        private var instance: VpnManager? = null

        fun getInstance(): VpnManager {
            if (instance == null) {
                instance = VpnManager()
            }
            return instance as VpnManager
        }
    }

    fun init(context: Context){
        this.context=context
        connect()
    }

    fun run() {
        when {
            state.canStop -> Core.stopService()
//            DataStore.serviceMode == Key.modeVpn -> {
//                val intent = VpnService.prepare(activity)
//                if (intent != null) activity.startActivityForResult(intent, REQUEST_CONNECT)
//                else onActivityResult(REQUEST_CONNECT, Activity.RESULT_OK, null)
//            }
            else -> Core.startService()
        }
    }

    
    fun setOnStatusChangeListener(listener: OnStatusChangeListener) {
        this.listener = listener
    }

    fun onStop() {
        connection.bandwidthTimeout = 0
    }

    fun onStart() {
        connection.bandwidthTimeout = 1000
    }
    
    fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        when {
            requestCode != REQUEST_CONNECT -> {
            }
            resultCode == Activity.RESULT_OK -> Core.startService()
            else -> {
                
            }
        }
    }

    private fun changeState(state: BaseService.State) {
        this.state = state
        this.listener?.onStatusChanged(state)
    }

    interface OnStatusChangeListener {
        fun onStatusChanged(state: BaseService.State)

        fun onTrafficUpdated(profileId: Long, stats: TrafficStats)
    }

    enum class Route(name: String) {
        
        ALL("all")
        
        ,
        BY_PASS_LAN("bypass-lan")
        
        ,
        BY_PASS_CHINA("bypass-china")
        
        ,
        BY_PASS_LAN_CHINA("bypass-lan-china")
    
        ,
        GFW_LIST("gfwlist")
        
        ,
        CHINA_LIST("china-list")
    
        ,
        CUSTOM_RULES("custom-rules");

        var route = name
    }
}
