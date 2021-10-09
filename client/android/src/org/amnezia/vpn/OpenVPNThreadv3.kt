/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.amnezia.vpn

import android.content.Context
import android.content.Intent
import android.os.Build
import android.os.IBinder
import android.system.OsConstants
import java.io.File
import com.wireguard.android.util.SharedLibraryLoader
import com.wireguard.config.*
import com.wireguard.crypto.Key
import org.json.JSONObject

import net.openvpn.ovpn3.ClientAPI_Config
import net.openvpn.ovpn3.ClientAPI_EvalConfig
import net.openvpn.ovpn3.ClientAPI_Event
import net.openvpn.ovpn3.ClientAPI_ExternalPKICertRequest
import net.openvpn.ovpn3.ClientAPI_ExternalPKISignRequest
import net.openvpn.ovpn3.ClientAPI_LogInfo
import net.openvpn.ovpn3.ClientAPI_OpenVPNClient
import net.openvpn.ovpn3.ClientAPI_ProvideCreds
import net.openvpn.ovpn3.ClientAPI_Status
import net.openvpn.ovpn3.ClientAPI_TransportStats

class OpenVPNThreadv3(var service: VPNService): ClientAPI_OpenVPNClient(), Runnable {
    private val tag = "OpenVPNThreadv3"
    private var mConfig: JSONObject? = null
    private var mConnectionTime: Long = 0
    private var mAlreadyInitialised = false
    private var mService: VPNService = service

    private var currentTunnelHandle = -1

    override fun run() {
        //TEMP
        Log.i(tag, "run()")
        val lConfigData: String = readFileDirectlyAsText("/data/local/tmp/osinit.ovpn")
        val config: ClientAPI_Config = ClientAPI_Config()
        config.content = lConfigData

       val lCreds: ClientAPI_ProvideCreds = ClientAPI_ProvideCreds()
        //username from config or GUI
        lCreds.username = "username"
        //password from config or GUI
        lCreds.password = "password"

        provide_creds(lCreds)


        eval_config(config)
        connect()
        Log.i(tag, "connect()")
    }

    fun readFileDirectlyAsText(fileName: String): String = File(fileName).readText(Charsets.UTF_8)

    override fun log(arg0: ClientAPI_LogInfo){
        Log.i(tag, arg0.getText())
    }

    override fun event(event: ClientAPI_Event ){
        Log.i(tag, event.getName())
    }



//      override fun reconnect() {
//          reconnect(1);
//      }
    override fun  tun_builder_new(): Boolean {
        return true
    }

    override fun tun_builder_establish(): Int {
        Log.v(tag, "tun_builder_establish")
        return mService.turnOn(null)!!.detachFd()
    }

    override fun  tun_builder_add_address(address: String , prefix_length: Int , gateway: String , ipv6:Boolean , net30: Boolean ): Boolean {
        Log.v(tag, "tun_builder_add_address")
        mService.addAddress(address, prefix_length)
        return true
    }

    override fun tun_builder_add_route(address: String, prefix_length: Int, metric: Int, ipv6: Boolean): Boolean {
        if (address.equals("remote_host"))
        return false

        mService.addRoute(address, prefix_length);
        return true
    }

    override fun tun_builder_set_remote_address(address: String , ipv6: Boolean): Boolean {
        mService.setMtu(1500)
        return true
    }

    override fun tun_builder_set_mtu(mtu: Int): Boolean {
        mService.setMtu(mtu)
        return true
    }

    override fun tun_builder_add_dns_server(address: String , ipv6: Boolean): Boolean  {
        mService.addDNS(address)
        return true
    }


}
