/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.amnezia.vpn

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.LocalSocket
import android.net.LocalSocketAddress
import android.net.Network
import android.net.ProxyInfo
import android.os.*
import android.system.ErrnoException
import android.system.Os
import android.system.OsConstants
import android.text.TextUtils
import androidx.core.content.FileProvider
import com.wireguard.android.util.SharedLibraryLoader
import com.wireguard.config.*
import com.wireguard.crypto.Key
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import org.amnezia.vpn.shadowsocks.core.Core
import org.amnezia.vpn.shadowsocks.core.R
import org.amnezia.vpn.shadowsocks.core.VpnRequestActivity
import org.amnezia.vpn.shadowsocks.core.acl.Acl
import org.amnezia.vpn.shadowsocks.core.bg.*
import org.amnezia.vpn.shadowsocks.core.database.Profile
import org.amnezia.vpn.shadowsocks.core.database.ProfileManager
import org.amnezia.vpn.shadowsocks.core.net.ConcurrentLocalSocketListener
import org.amnezia.vpn.shadowsocks.core.net.DefaultNetworkListener
import org.amnezia.vpn.shadowsocks.core.net.Subnet
import org.amnezia.vpn.shadowsocks.core.preference.DataStore
import org.amnezia.vpn.shadowsocks.core.utils.Key.modeVpn
import org.json.JSONObject
import java.io.Closeable
import java.io.File
import java.io.FileDescriptor
import java.io.IOException
import java.lang.Exception
import java.util.Locale
import android.net.VpnService as BaseVpnService


class VPNService : BaseVpnService(), BaseService.Interface {

    override val data = BaseService.Data(this)
    override val tag: String get() = "VPNService"


    private var conn: ParcelFileDescriptor? = null
    private var worker: ProtectWorker? = null
    private var active = false
    private var metered = false
    private var underlyingNetwork: Network? = null
        set(value) {
            field = value
            if (active && Build.VERSION.SDK_INT >= 22) setUnderlyingNetworks(underlyingNetworks)
        }
    private val underlyingNetworks
        get() =
            // clearing underlyingNetworks makes Android 9+ consider the network to be metered
            if (Build.VERSION.SDK_INT >= 28 && metered) null else underlyingNetwork?.let {
                arrayOf(
                    it
                )
            }

    val handler = Handler(Looper.getMainLooper())
    var runnable: Runnable = object : Runnable {
        override fun run() {
            if (mProtocol.equals("shadowsocks", true)) {
                Log.e(tag, "run:  -----------------: ${data.state}")
                when (data.state) {
                    BaseService.State.Connected -> {
                        currentTunnelHandle = 1
                        isUp = true
                    }
                    BaseService.State.Stopped -> {
                        currentTunnelHandle = -1
                        isUp = false
                    }
                    else -> {

                    }
                }
            }
            handler.postDelayed(this, 1000L) //wait 4 sec and run again
        }
    }

    fun stopTest() {
        handler.removeCallbacks(runnable)
    }


    fun startTest() {
        handler.postDelayed(runnable, 0) //wait 0 ms and run
    }

    companion object {
        private const val VPN_MTU = 1500
        private const val PRIVATE_VLAN4_CLIENT = "172.19.0.1"
        private const val PRIVATE_VLAN4_ROUTER = "172.19.0.2"
        private const val PRIVATE_VLAN6_CLIENT = "fdfe:dcba:9876::1"
        private const val PRIVATE_VLAN6_ROUTER = "fdfe:dcba:9876::2"

        /**
         * https://android.googlesource.com/platform/prebuilts/runtime/+/94fec32/appcompat/hiddenapi-light-greylist.txt#9466
         */
        private val getInt = FileDescriptor::class.java.getDeclaredMethod("getInt$")

 	private fun <T> FileDescriptor.use(block: (FileDescriptor) -> T) = try {
		    block(this)
		} finally {
		    try {
		        Os.close(this)
		    } catch (_: ErrnoException) { }
		}

        @JvmStatic
        fun startService(c: Context) {
            c.applicationContext.startService(
                Intent(c.applicationContext, VPNService::class.java).apply {
                    putExtra("startOnly", true)
                })
        }

        @JvmStatic
        private external fun wgGetConfig(handle: Int): String?

        @JvmStatic
        private external fun wgGetSocketV4(handle: Int): Int

        @JvmStatic
        private external fun wgGetSocketV6(handle: Int): Int

        @JvmStatic
        private external fun wgTurnOff(handle: Int)

        @JvmStatic
        private external fun wgTurnOn(ifName: String, tunFd: Int, settings: String): Int

        @JvmStatic
        private external fun wgVersion(): String?
    }

    private var mBinder: VPNServiceBinder = VPNServiceBinder(this)
    private var mConfig: JSONObject? = null
    private var mProtocol: String? = null
    private var mConnectionTime: Long = 0
    private var mAlreadyInitialised = false
    private var mbuilder: Builder = Builder()

    private var mOpenVPNThreadv3: OpenVPNThreadv3? = null
    var currentTunnelHandle = -1

    private var intent: Intent? = null
    private var flags = 0
    private var startId = 0

    fun init() {
        if (mAlreadyInitialised) {
            return
        }
        Log.init(this)
        SharedLibraryLoader.loadSharedLibrary(this, "wg-go")
        SharedLibraryLoader.loadSharedLibrary(this, "ovpn3")
        Log.i(tag, "Loaded libs")
        Log.e(tag, "Wireguard Version ${wgVersion()}")
        mOpenVPNThreadv3 = OpenVPNThreadv3(this)
        mAlreadyInitialised = true
    }

    override fun onCreate() {
        super.onCreate()
    }

    override fun onUnbind(intent: Intent?): Boolean {
        Log.v(tag, "Aman: onUnbind....................")
        if (!isUp) {
            // If the Qt Client got closed while we were not connected
            // we do not need to stay as a foreground service.
            stopForeground(true)
        }
        return super.onUnbind(intent)
    }

    override fun onDestroy() {
        turnOff()

        super.onDestroy()
    }

    /**
     * EntryPoint for the Service, gets Called when AndroidController.cpp
     * calles bindService. Returns the [VPNServiceBinder] so QT can send Requests to it.
     */
    override fun onBind(intent: Intent): IBinder {
        Log.v(tag, "Aman: onBind....................")

	// This start is from always-on
        if (this.mConfig == null) {
        
            // We don't have tunnel to turn on - Try to create one with last config the service got
            val prefs = Prefs.get(this)
            val lastConfString = prefs.getString("lastConf", "")
            if (lastConfString.isNullOrEmpty()) {
                // We have nothing to connect to -> Exit
                Log.e(tag, "VPN service was triggered without defining a Server or having a tunnel")
                init()
                return mBinder
            }
            this.mConfig = JSONObject(lastConfString)
        }
	
        mProtocol = mConfig!!.getString("protocol")
        Log.e(tag, "mProtocol: $mProtocol")
       
        when (mProtocol) {
            "shadowsocks" -> {
                startShadowsocks()
                startTest()
            }
            else -> {
                init()
            }
        }

        return mBinder
    }

    /**
     * Might be the entryPoint if the Service gets Started via an
     * Service Intent: Might be from Always-On-Vpn from Settings
     * or from Booting the device and having "connect on boot" enabled.
     */
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        Log.v(tag, "Aman: onStartCommand....................")
        this.intent = intent
        this.flags = flags
        this.startId = startId
        init()
        intent?.let {
            if (!isUp && intent.getBooleanExtra("startOnly", false)) {
                Log.i(tag, "Start only!")
                return START_REDELIVER_INTENT
//                return super<LocalDnsService.Interface>.onStartCommand(intent, flags, startId)
            }
        }
        // This start is from always-on
        if (this.mConfig == null) {
            // We don't have tunnel to turn on - Try to create one with last config the service got
            val prefs = Prefs.get(this)
            val lastConfString = prefs.getString("lastConf", "")
            if (lastConfString.isNullOrEmpty()) {
                // We have nothing to connect to -> Exit
                Log.e(tag, "VPN service was triggered without defining a Server or having a tunnel")
                return super<android.net.VpnService>.onStartCommand(intent, flags, startId)
            }
            this.mConfig = JSONObject(lastConfString)
        }

        mProtocol = mConfig!!.getString("protocol")
        Log.e(tag, "mProtocol: $mProtocol")
        if (mProtocol.equals("shadowsocks", true)) {
		if (prepare(this) != null) {
                startActivity(Intent(this, VpnRequestActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
            } else return super<BaseService.Interface>.onStartCommand(intent, flags, startId)
        
        stopRunner()
        return START_NOT_STICKY
        }
        return START_REDELIVER_INTENT
    }

    // Invoked when the application is revoked.
    // At this moment, the VPN interface is already deactivated by the system.
    override fun onRevoke() {
        Log.v(tag, "Aman: onRevoke....................")
        this.turnOff()
        super.onRevoke()
    }

    var connectionTime: Long = 0
        get() {
            return mConnectionTime
        }

    var isUp: Boolean = false
        get() {
            return when (mProtocol) {
                "openvpn" -> {
                    field
                }
                else -> {
                    currentTunnelHandle >= 0
                }
            }
        }
        set(value) {
            if (value) {
                mBinder.dispatchEvent(VPNServiceBinder.EVENTS.connected, "")
                mConnectionTime = System.currentTimeMillis()
                return
            }
            mBinder.dispatchEvent(VPNServiceBinder.EVENTS.disconnected, "")
            mConnectionTime = 0
        }

    val status: JSONObject
        get() {
            val deviceIpv4: String = ""

            val status = when (mProtocol) {
                "openvpn" -> {
                    if (mOpenVPNThreadv3 == null) {
                        Status(null, null, null, null)
                    } else {
                        val rx = mOpenVPNThreadv3?.getTotalRxBytes() ?: ""
                        val tx = mOpenVPNThreadv3?.getTotalTxBytes() ?: ""

                        Status(
                            rx.toString(),
                            tx.toString(),
                            if (mConfig!!.has("server")) { mConfig?.getJSONObject("server")?.getString("ipv4Gateway") } else {""},
                            if (mConfig!!.has("device")) { mConfig?.getJSONObject("device")?.getString("ipv4Address") } else {""}
                        )
                    }
                }
                else -> {
                    Status(
                        getConfigValue("rx_bytes"),
                        getConfigValue("tx_bytes"),
                        if (mConfig!!.has("server")) { mConfig?.getJSONObject("server")?.getString("ipv4Gateway") } else {""},
                        if (mConfig!!.has("server")) {mConfig?.getJSONObject("device")?.getString("ipv4Address") } else {""}
                    )
                }
            }

            return JSONObject().apply {
                putOpt("rx_bytes", status.rxBytes)
                putOpt("tx_bytes", status.txBytes)
                putOpt("endpoint", status.endpoint)
                putOpt("deviceIpv4", status.device)
            }
        }

    data class Status(
        var rxBytes: String?,
        var txBytes: String?,
        var endpoint: String?,
        var device: String?
    )

    /*
    * Checks if the VPN Permission is given.
    * If the permission is given, returns true
    * Requests permission and returns false if not.
    */
    fun checkPermissions(): Boolean {
        // See https://developer.android.com/guide/topics/connectivity/vpn#connect_a_service
        // Call Prepare, if we get an Intent back, we dont have the VPN Permission
        // from the user. So we need to pass this to our main Activity and exit here.
        val intent = prepare(this)
        if (intent == null) {
            Log.e(tag, "VPN Permission Already Present")
            return true
        }
        Log.e(tag, "Requesting VPN Permission")
        return false
    }

    fun turnOn(json: JSONObject?): Int {
        Log.v(tag, "Aman: turnOn....................")
        if (!checkPermissions()) {
            Log.e(tag, "turn on was called without no permissions present!")
            isUp = false
            return 0
        }
        Log.i(tag, "Permission okay")
        mConfig = json!!
        Log.i(tag, "Config: $mConfig")
        mProtocol = mConfig!!.getString("protocol")
        Log.i(tag, "Protocol: $mProtocol")

        when (mProtocol) {
            "cloak",
            "openvpn" -> {
                startOpenVpn()
            }
            "wireguard" -> {
                startWireGuard()
            }
            "shadowsocks" -> {
                startShadowsocks()
                //startTest()
            }
            else -> {
                Log.e(tag, "No protocol")
                return 0
            }
        }
        NotificationUtil.show(this)
        return 1
    }

    fun establish(): ParcelFileDescriptor? {
        mbuilder.allowFamily(OsConstants.AF_INET)
        mbuilder.allowFamily(OsConstants.AF_INET6)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) mbuilder.setMetered(false)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) setUnderlyingNetworks(null)

        return mbuilder.establish()
    }

    fun setMtu(mtu: Int) {
        mbuilder.setMtu(mtu)
    }

    fun addAddress(ip: String, len: Int) {
        Log.v(tag, "mbuilder.addAddress($ip, $len)")
        mbuilder.addAddress(ip, len)
    }

    fun addRoute(ip: String, len: Int) {
        Log.v(tag, "mbuilder.addRoute($ip, $len)")
        mbuilder.addRoute(ip, len)
    }

    fun addDNS(ip: String) {
        Log.v(tag, "mbuilder.addDnsServer($ip)")
        mbuilder.addDnsServer(ip)
        if ("samsung".equals(Build.BRAND) && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            mbuilder.addRoute(ip, 32)
        }
    }

    fun setSessionName(name: String) {
        Log.v(tag, "mbuilder.setSession($name)")
        mbuilder.setSession(name)
    }

    fun addHttpProxy(host: String, port: Int): Boolean {
        val proxyInfo = ProxyInfo.buildDirectProxy(host, port)
        Log.v(tag, "mbuilder.addHttpProxy($host, $port)")
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            mbuilder.setHttpProxy(proxyInfo)
        }
        return true
    }

    fun setDomain(domain: String) {
        Log.v(tag, "mbuilder.setDomain($domain)")
        mbuilder.addSearchDomain(domain)
    }

    fun turnOff() {
        Log.v(tag, "Aman: turnOff....................")
        when (mProtocol) {
            "wireguard" -> {
                wgTurnOff(currentTunnelHandle)
            }
            "cloak",
            "openvpn" -> {
                ovpnTurnOff()
            }
            "shadowsocks" -> {
                stopRunner(false)
                stopTest()
            }
            else -> {
                Log.e(tag, "No protocol")
            }
        }

        currentTunnelHandle = -1
        stopForeground(true)
        isUp = false
        stopSelf()
    }


    private fun ovpnTurnOff() {
        mOpenVPNThreadv3?.stop()
        mOpenVPNThreadv3 = null
        Log.e(tag, "mOpenVPNThreadv3 stop!")
    }

    /**
     * Configures an Android VPN Service Tunnel
     * with a given Wireguard Config
     */
    private fun setupBuilder(config: Config, builder: Builder) {
        // Setup Split tunnel
        for (excludedApplication in config.`interface`.excludedApplications)
            builder.addDisallowedApplication(excludedApplication)

        // Device IP
        for (addr in config.`interface`.addresses) builder.addAddress(addr.address, addr.mask)
        // DNS
        for (addr in config.`interface`.dnsServers) builder.addDnsServer(addr.hostAddress)
        // Add All routes the VPN may route tos
        for (peer in config.peers) {
            for (addr in peer.allowedIps) {
                builder.addRoute(addr.address, addr.mask)
            }
        }
        builder.allowFamily(OsConstants.AF_INET)
        builder.allowFamily(OsConstants.AF_INET6)
        builder.setMtu(config.`interface`.mtu.orElse(1280))

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) builder.setMetered(false)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) setUnderlyingNetworks(null)

        builder.setBlocking(true)
    }

    /**
     * Gets config value for {key} from the Current
     * running Wireguard tunnel
     */
    private fun getConfigValue(key: String): String? {
        if (!isUp) {
            return null
        }
        val config = wgGetConfig(currentTunnelHandle) ?: return null
        val lines = config.split("\n")
        for (line in lines) {
            val parts = line.split("=")
            val k = parts.first()
            val value = parts.last()
            if (key == k) {
                return value
            }
        }
        return null
    }

    private fun parseConfigData(data: String): Map<String, Map<String, String>> {
        val parseData = mutableMapOf<String, Map<String, String>>()
        var currentSection: Pair<String, MutableMap<String, String>>? = null
        data.lines().forEach { line ->
            if (line.isNotEmpty()) {
                if (line.startsWith('[')) {
                    currentSection?.let {
                        parseData.put(it.first, it.second)
                    }
                    currentSection =
                        line.substring(1, line.indexOfLast { it == ']' }) to mutableMapOf()
                } else {
                    val parameter = line.split("=", limit = 2)
                    currentSection!!.second.put(parameter.first().trim(), parameter.last().trim())
                }
            }
        }
        currentSection?.let {
            parseData.put(it.first, it.second)
        }
        return parseData
    }

    /**
     * Create a Wireguard [Config]  from a [json] string -
     * The [json] will be created in AndroidVpnProtocol.cpp
     */
    private fun buildWireugardConfig(obj: JSONObject): Config {
        val confBuilder = Config.Builder()
        val wireguardConfigData = obj.getJSONObject("wireguard_config_data")
        val config = parseConfigData(wireguardConfigData.getString("config"))
        val peerBuilder = Peer.Builder()
        val peerConfig = config["Peer"]!!
        peerBuilder.setPublicKey(Key.fromBase64(peerConfig["PublicKey"]))
        peerConfig["PresharedKey"]?.let {
            peerBuilder.setPreSharedKey(Key.fromBase64(it))
        }
        val allowedIPList = peerConfig["AllowedIPs"]?.split(",") ?: emptyList()
        if (allowedIPList.isEmpty()) {
            val internet = InetNetwork.parse("0.0.0.0/0") // aka The whole internet.
            peerBuilder.addAllowedIp(internet)
        } else {
            allowedIPList.forEach {
                val network = InetNetwork.parse(it.trim())
                peerBuilder.addAllowedIp(network)
            }
        }
        val endpointConfig = peerConfig["Endpoint"]
        val endpoint = InetEndpoint.parse(endpointConfig)
        peerBuilder.setEndpoint(endpoint)
        peerConfig["PersistentKeepalive"]?.let {
            peerBuilder.setPersistentKeepalive(it.toInt())
        }
        confBuilder.addPeer(peerBuilder.build())

        val ifaceBuilder = Interface.Builder()
        val ifaceConfig = config["Interface"]!!
        ifaceBuilder.parsePrivateKey(ifaceConfig["PrivateKey"])
        ifaceBuilder.addAddress(InetNetwork.parse(ifaceConfig["Address"]))
        ifaceConfig["DNS"]!!.split(",").forEach {
            ifaceBuilder.addDnsServer(InetNetwork.parse(it.trim()).address)
        }

        confBuilder.setInterface(ifaceBuilder.build())

        return confBuilder.build()
    }

    fun getVpnConfig(): JSONObject {
        return mConfig!!
    }

    private fun startShadowsocks() {
        Log.e(tag, "startShadowsocks method enters")
        if (mConfig != null) {
            try {
                Log.e(tag, "Config: $mConfig")

                ProfileManager.clear()
                val profile = Profile()
                
                val shadowsocksConfig = mConfig?.getJSONObject("shadowsocks_config_data")

                if (shadowsocksConfig?.has("name") == true) {
                    profile.name = shadowsocksConfig.getString("name")
                } else {
                    profile.name = "amnezia"
                }
                if (shadowsocksConfig?.has("method") == true) {
                    profile.method = shadowsocksConfig.getString("method").toString()
                }
                if (shadowsocksConfig?.has("server") == true) {
                    profile.host = shadowsocksConfig.getString("server").toString()
                    Log.i(tag, "startShadowsocks: profile.host" + profile.host)
                    
                }
                if (shadowsocksConfig?.has("password") == true) {
                    profile.password = shadowsocksConfig.getString("password").toString()
                }
                if (shadowsocksConfig?.has("server_port") == true) {
                    profile.remotePort = shadowsocksConfig.getInt("server_port")
                    Log.i(tag, "startShadowsocks: profile.remotePort" + profile.remotePort)
                    
                }
                
                 val prefs = Prefs.get(this)
		 prefs.edit()
		    .putString("lastConf", mConfig.toString())
		    .apply()


                profile.proxyApps = false
                profile.bypass = false
                profile.metered = false
                profile.dirty = false
                profile.ipv6 = true

                DataStore.profileId = ProfileManager.createProfile(profile).id
                val switchProfile = Core.switchProfile(DataStore.profileId)
                Log.i(tag, "startShadowsocks: SwitchProfile: $switchProfile")
                intent?.putExtra("startOnly", false)
                onStartCommand(
                    intent,
                    flags,
                    startId
                )

            } catch (e: Exception) {
                Log.e(tag, "Error in startShadowsocks: $e")
            }
        } else {
            Log.e(tag, "Invalid config file!!")
        }
    }

    private fun startOpenVpn() {
        mOpenVPNThreadv3 = OpenVPNThreadv3(this)

        Thread({
            mOpenVPNThreadv3?.run()
        }).start()
    }

    private fun startWireGuard() {
        val wireguard_conf = buildWireugardConfig(mConfig!!)
        Log.i(tag, "startWireGuard: wireguard_conf : $wireguard_conf")
        if (currentTunnelHandle != -1) {
            Log.e(tag, "Tunnel already up")
            // Turn the tunnel down because this might be a switch
            wgTurnOff(currentTunnelHandle)
        }
        val wgConfig: String = wireguard_conf.toWgUserspaceString()
        val builder = Builder()
        setupBuilder(wireguard_conf, builder)
        builder.setSession("Amnezia")
        builder.establish().use { tun ->
            if (tun == null) return
            Log.i(tag, "Go backend " + wgVersion())
            currentTunnelHandle = wgTurnOn("Amnezia", tun.detachFd(), wgConfig)
        }
        if (currentTunnelHandle < 0) {
            Log.e(tag, "Activation Error Code -> $currentTunnelHandle")
            isUp = false
            return
        }
        protect(wgGetSocketV4(currentTunnelHandle))
        protect(wgGetSocketV6(currentTunnelHandle))
        isUp = true

        // Store the config in case the service gets
        // asked boot vpn from the OS
        val prefs = Prefs.get(this)
        prefs.edit()
            .putString("lastConf", mConfig.toString())
            .apply()
    }

    override suspend fun startProcesses() {
        worker = ProtectWorker().apply { start() }
        try {
            Log.i(tag, "startProcesses: ------------------1")
            super.startProcesses()
            Log.i(tag, "startProcesses: ------------------2")
            sendFd(startVpn())
            Log.i(tag, "startProcesses: ------------------3")
            
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    override fun killProcesses(scope: CoroutineScope) {
        super.killProcesses(scope)
        active = false
        scope.launch { DefaultNetworkListener.stop(this) }
        worker?.shutdown(scope)
        worker = null
        conn?.close()
        conn = null
    }

    private suspend fun startVpn(): FileDescriptor {
        val profile = data.proxy!!.profile
        Log.i(tag, "startVpn: -----------------------1")
        val builder = Builder()
                .setConfigureIntent(Core.configureIntent(this))
                .setSession(profile.formattedName)
                .setMtu(VPN_MTU)
                .addAddress(PRIVATE_VLAN4_CLIENT, 30)
                .addDnsServer("8.8.8.8")
            
        Log.i(tag, "startVpn: -----------------------3")
        
        if (profile.ipv6) builder.addAddress(PRIVATE_VLAN6_CLIENT, 126)

        if (profile.proxyApps) {
            val me = packageName
            profile.individual.split('\n')
                    .filter { it != me }
                    .forEach {
                        try {
                            if (profile.bypass) builder.addDisallowedApplication(it)
                            else builder.addAllowedApplication(it)
                        } catch (ex: PackageManager.NameNotFoundException) {
                            Log.i(tag, ex.message.toString())
                        }
                    }
            if (!profile.bypass) builder.addAllowedApplication(me)
        }
        val me = packageName
        builder.addDisallowedApplication(me)
        
        Log.i(tag, "startVpn: -----------------------4")
        when (profile.route) {
            Acl.ALL, Acl.BYPASS_CHN, Acl.CUSTOM_RULES -> {
            	Log.i(tag, "add route 0.0.0.0")
                builder.addRoute("0.0.0.0", 0)
                if (profile.ipv6) builder.addRoute("::", 0)
            }
            else -> {
            	Log.i(tag, "else add route 0.0.0.0")
                builder.addRoute("0.0.0.0", 0)
                if (profile.ipv6) builder.addRoute("::", 0)
                resources.getStringArray(R.array.bypass_private_route).forEach {
                    val subnet = Subnet.fromString(it)!!
                    builder.addRoute(subnet.address.hostAddress!!, subnet.prefixSize)
                }
                builder.addRoute(PRIVATE_VLAN4_ROUTER, 32)
                // https://issuetracker.google.com/issues/149636790
                if (profile.ipv6) builder.addRoute("2000::", 3)
            }
        }
        
        metered = profile.metered
        active = true   // possible race condition here?
        builder.setUnderlyingNetworks(underlyingNetworks)
        if (Build.VERSION.SDK_INT >= 29) builder.setMetered(metered)

        val conn = builder.establish() ?: throw NullConnectionException()
        this.conn = conn
        
        
        val cmd = arrayListOf(File(applicationInfo.nativeLibraryDir, Executable.TUN2SOCKS).absolutePath,
                "--netif-ipaddr", PRIVATE_VLAN4_ROUTER,
                "--socks-server-addr", "${DataStore.listenAddress}:${DataStore.portProxy}",
                "--tunmtu", VPN_MTU.toString(),
                "--sock-path", File(Core.deviceStorage.noBackupFilesDir, "sock_path").absolutePath,
                "--dnsgw", "127.0.0.1:${DataStore.portLocalDns}",
                "--loglevel", "info")
        if (profile.ipv6) {
            cmd += "--netif-ip6addr"
            cmd += PRIVATE_VLAN6_ROUTER
        }
        cmd += "--enable-udprelay"
        
        Log.i(tag, "startVpn: -----------------------12")
        data.processes!!.start(cmd, onRestartCallback = {
            try {
                sendFd(conn.fileDescriptor)
            } catch (e: ErrnoException) {
                e.printStackTrace()
                stopRunner(false, e.message)
            }
        })
        Log.i(tag, "startVpn: -----------------------13")
        return conn.fileDescriptor
    }

    private suspend fun sendFd(fd: FileDescriptor) {
        var tries = 0
        val path = File(Core.deviceStorage.noBackupFilesDir, "sock_path").absolutePath
        while (true) try {
            delay(500L shl tries)
            LocalSocket().use { localSocket ->
                localSocket.connect(
                    LocalSocketAddress(
                        path,
                        LocalSocketAddress.Namespace.FILESYSTEM
                    )
                )
                localSocket.setFileDescriptorsForSend(arrayOf(fd))
                localSocket.outputStream.write(42)
            }
            return
        } catch (e: IOException) {
            if (tries > 5) throw e
            tries += 1
        }
    }

    private inner class ProtectWorker : ConcurrentLocalSocketListener("ShadowsocksVpnThread",
            File(Core.deviceStorage.noBackupFilesDir, "protect_path")) {
        override fun acceptInternal(socket: LocalSocket) {
            if (socket.inputStream.read() == -1) return
            val success = socket.ancillaryFileDescriptors!!.single()!!.use { fd ->
                underlyingNetwork.let { network ->
                    if (network != null) try {
                        network.bindSocket(fd)
                        return@let true
                    } catch (e: IOException) {
                        when ((e.cause as? ErrnoException)?.errno) {
                            OsConstants.EPERM, OsConstants.EACCES, OsConstants.ENONET -> Log.e(tag, e.message.toString())
                            else -> Log.e(tag, e.message.toString())
                        }
                        return@let false
                    }
                    protect(getInt.invoke(fd) as Int)
                }
            }
            try {
                socket.outputStream.write(if (success) 0 else 1)
            } catch (_: IOException) { }        // ignore connection early close
        }
    }

    inner class NullConnectionException : NullPointerException() {
        override fun getLocalizedMessage() = getString(R.string.reboot_required)
    }

    class CloseableFd(val fd: FileDescriptor) : Closeable {
        override fun close() = Os.close(fd)
    }

    fun saveAsFile(configContent: String?, suggestedFileName: String): String {
        val rootDirPath = cacheDir.absolutePath
        val rootDir = File(rootDirPath)

        if (!rootDir.exists()) {
            rootDir.mkdirs()
        }

        val fileName = if (!TextUtils.isEmpty(suggestedFileName)) suggestedFileName else "amnezia.cfg"

        val file = File(rootDir, fileName)

        try {
            file.bufferedWriter().use { out -> out.write(configContent) }
            return file.toString()
        } catch (e: Exception) {
            e.printStackTrace()
        }

        return ""
    }

    fun shareFile(attachmentFile: String?) {
        try {
            val intent = Intent(Intent.ACTION_SEND)
            intent.type = "text/*"
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)

            val file = File(attachmentFile)
            val uri = FileProvider.getUriForFile(this, "${BuildConfig.APPLICATION_ID}.fileprovider", file)
            intent.putExtra(Intent.EXTRA_STREAM, uri)
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)

            val createChooser = Intent.createChooser(intent, "Config sharing")
            createChooser.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            startActivity(createChooser)
        } catch (e: Exception) {
            Log.i(tag, e.message.toString())
        }
    }
}
