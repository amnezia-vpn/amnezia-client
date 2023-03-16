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
import java.util.Base64

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
import java.lang.StringBuilder

class OpenVPNThreadv3(var service: VPNService): ClientAPI_OpenVPNClient(), Runnable {
    private val tag = "OpenVPNThreadv3"
    private var mAlreadyInitialised = false
    private var mService: VPNService = service

    private var bytesInIndex = -1
    private var bytesOutIndex = -1

    init {
        findConfigIndicies()
    }

    private fun findConfigIndicies() {
        val n: Int = stats_n()

        for (i in 0 until n) {
            val name: String = stats_name(i)
            if (name == "BYTES_IN") bytesInIndex = i
            if (name == "BYTES_OUT") bytesOutIndex = i
        }
    }

    fun getTotalRxBytes(): Long {
        return stats_value(bytesInIndex)
    }

    fun getTotalTxBytes(): Long {
        return stats_value(bytesOutIndex)
    }

    override fun run() {

        val config: ClientAPI_Config = ClientAPI_Config()

        val jsonVpnConfig = mService.getVpnConfig()
        val ovpnConfig = jsonVpnConfig.getJSONObject("openvpn_config_data").getString("config")

        val resultingConfig = StringBuilder()
        resultingConfig.append(ovpnConfig)

        if (jsonVpnConfig.getString("protocol") == "cloak") {
            val cloakConfigJson: JSONObject = jsonVpnConfig.getJSONObject("cloak_config_data")

            if (cloakConfigJson.keySet().contains("NumConn")) {
                cloakConfigJson.put("NumConn", 1)
            }

            if (cloakConfigJson.keySet().contains("ProxyMethod")) {
                cloakConfigJson.put("ProxyMethod", "openvpn")
            }

            if (cloakConfigJson.keySet().contains("port")) {
                val portValue = cloakConfigJson.get("port")
                cloakConfigJson.remove("port")
                cloakConfigJson.put("RemotePort", portValue)
            }

            if (cloakConfigJson.keySet().contains("remote")) {
                val hostValue = cloakConfigJson.get("remote")
                cloakConfigJson.remove("remote")
                cloakConfigJson.put("RemoteHost", hostValue)
            }

            val cloakConfig = Base64.getEncoder().encodeToString(
                jsonVpnConfig.getJSONObject("cloak_config_data").toString().toByteArray()
            )

            resultingConfig.append("\n<cloak>\n")
            resultingConfig.append(cloakConfig)
            resultingConfig.append("\n</cloak>\n")

            config.setUsePluggableTransports(true)
        }
        
        Log.i(tag, "resultingConfig() " + resultingConfig.toString())
        
        if (jsonVpnConfig.getString("protocol") == "shadowsocks") {
            val socksIndex = resultingConfig.indexOf("socks-proxy")
            val socksEndIndex =  resultingConfig.indexOf("\n", socksIndex)
            resultingConfig.delete(socksIndex, socksEndIndex)
            
            val remoteIndex = resultingConfig.indexOf("remote ")
            val remoteEndIndex = resultingConfig.indexOf("\n", remoteIndex)
            
            resultingConfig.insert(remoteEndIndex, "\nlocal 127.0.0.1\n") 
            
            val portStart = resultingConfig.lastIndexOf(" ", remoteEndIndex)
            
            resultingConfig.delete(remoteIndex, portStart)
            
            resultingConfig.insert(remoteIndex, "remote 172.19.0.2 ")
            
            
            
            Log.i(tag, "shadowsocks() " + resultingConfig.toString())
            
        }


        config.content = resultingConfig.toString()

        eval_config(config)

        val status = connect()
        if (status.getError()) {
            Log.i(tag, "connect() error: " + status.getError() + ": " + status.getMessage())
        }
    }

    override fun log(arg0: ClientAPI_LogInfo){
        Log.i(tag, arg0.getText())
    }

    override fun event(event: ClientAPI_Event ) {
        val eventName = event.getName()
        when (eventName) {
            "CONNECTED" -> mService.isUp = true
            "DISCONNECTED" -> mService.isUp = false
        }
        Log.i(tag, eventName)
    }

    override fun tun_builder_new(): Boolean {
        return true
    }

    override fun tun_builder_establish(): Int {
        Log.v(tag, "tun_builder_establish")
        return mService.establish()!!.detachFd()
    }

    override fun  tun_builder_add_address(address: String , prefix_length: Int , gateway: String , ipv6:Boolean , net30: Boolean ): Boolean {
        Log.v(tag, "tun_builder_add_address")
        mService.addAddress(address, prefix_length)
        return true
    }

    override fun tun_builder_add_route(address: String, prefix_length: Int, metric: Int, ipv6: Boolean): Boolean {
        Log.v(tag, "tun_builder_add_route")
        if (address.equals("remote_host"))
        return false

        mService.addRoute(address, prefix_length);
        return true
    }

    override fun tun_builder_reroute_gw(ipv4: Boolean, ipv6: Boolean , flags: Long): Boolean {
          Log.v(tag, "tun_builder_reroute_gw")
          mService.addRoute("0.0.0.0", 0)
          return true
    }

    override fun tun_builder_exclude_route(address: String, prefix_length: Int, metric: Int, ipv6: Boolean): Boolean {
        Log.v(tag, "tun_builder_exclude_route")
        mService.addRoute(address, prefix_length);
        return true
    }

    override fun tun_builder_set_remote_address(address: String , ipv6: Boolean): Boolean {
        mService.setMtu(1500)
        return true
    }

    override fun tun_builder_set_mtu(mtu: Int): Boolean {
        Log.v(tag, "tun_builder_set_mtu")
        mService.setMtu(mtu)
        return true
    }

    override fun tun_builder_add_dns_server(address: String , ipv6: Boolean): Boolean  {
        mService.addDNS(address)
        return true
    }

    override fun tun_builder_set_session_name(name: String ): Boolean {
        Log.v(tag, "We should call this session: " + name)
        mService.setSessionName(name)
        return true
    }

    override fun tun_builder_set_proxy_http(host: String, port: Int): Boolean {
          return mService.addHttpProxy(host, port);
    }

    override fun tun_builder_set_proxy_https(host: String , port: Int): Boolean {
       Log.v(tag, "tun_builder_set_proxy_https")
       return false
    }

    override fun pause_on_connection_timeout(): Boolean {
       Log.v(tag, "pause_on_connection_timeout")
       return true
    }

    override fun tun_builder_add_search_domain(domain: String ): Boolean {
           mService.setDomain(domain);
           return true
    }

    override fun tun_builder_set_layer(layer: Int): Boolean {
        return layer == 3
    }

    override fun  socket_protect(socket: Int, remote: String, ipv6: Boolean): Boolean {
        Log.v(tag, "socket_protect")
        return mService.protect(socket);

    }

    override fun stop() {
        super.stop()
    }
}
