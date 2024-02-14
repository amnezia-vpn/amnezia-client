package org.amnezia.vpn.protocol

import android.annotation.SuppressLint
import android.content.Context
import android.net.IpPrefix
import android.net.VpnService
import android.net.VpnService.Builder
import android.os.Build
import android.system.OsConstants
import androidx.annotation.RequiresApi
import java.io.File
import java.io.FileOutputStream
import java.util.zip.ZipFile
import kotlinx.coroutines.flow.MutableStateFlow
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.net.InetNetwork
import org.json.JSONObject

private const val TAG = "Protocol"

const val VPN_SESSION_NAME = "AmneziaVPN"

private const val SPLIT_TUNNEL_DISABLE = 0
private const val SPLIT_TUNNEL_INCLUDE = 1
private const val SPLIT_TUNNEL_EXCLUDE = 2

abstract class Protocol {

    abstract val statistics: Statistics
    protected lateinit var state: MutableStateFlow<ProtocolState>
    protected lateinit var onError: (String) -> Unit

    open fun initialize(context: Context, state: MutableStateFlow<ProtocolState>, onError: (String) -> Unit) {
        this.state = state
        this.onError = onError
    }

    abstract fun startVpn(config: JSONObject, vpnBuilder: Builder, protect: (Int) -> Boolean)

    abstract fun stopVpn()

    abstract fun reconnectVpn(vpnBuilder: Builder)

    protected fun ProtocolConfig.Builder.configSplitTunneling(config: JSONObject) {
        if (!allowSplitTunneling) {
            Log.i(TAG, "Global address split tunneling is prohibited, " +
                "only tunneling from the protocol config is used")
            return
        }

        val splitTunnelType = config.optInt("splitTunnelType")
        if (splitTunnelType == SPLIT_TUNNEL_DISABLE) return
        val splitTunnelSites = config.getJSONArray("splitTunnelSites")
        val addressHandlerFunc = when (splitTunnelType) {
            SPLIT_TUNNEL_INCLUDE -> ::includeAddress
            SPLIT_TUNNEL_EXCLUDE -> ::excludeAddress

            else -> throw BadConfigException("Unexpected value of the 'splitTunnelType' parameter: $splitTunnelType")
        }

        for (i in 0 until splitTunnelSites.length()) {
            val address = InetNetwork.parse(splitTunnelSites.getString(i))
            addressHandlerFunc(address)
        }
    }

    protected open fun buildVpnInterface(config: ProtocolConfig, vpnBuilder: Builder) {
        vpnBuilder.setSession(VPN_SESSION_NAME)

        for (addr in config.addresses) {
            Log.d(TAG, "addAddress: $addr")
            vpnBuilder.addAddress(addr)
        }

        for (addr in config.dnsServers) {
            Log.d(TAG, "addDnsServer: $addr")
            vpnBuilder.addDnsServer(addr)
        }
        // fix for Samsung android ignoring DNS servers outside the VPN route range
        if (Build.BRAND == "samsung") {
            for (addr in config.dnsServers) {
                Log.d(TAG, "addRoute: $addr")
                vpnBuilder.addRoute(InetNetwork(addr))
            }
        }

        config.searchDomain?.let {
            Log.d(TAG, "addSearchDomain: $it")
            vpnBuilder.addSearchDomain(it)
        }

        for (addr in config.routes) {
            Log.d(TAG, "addRoute: $addr")
            vpnBuilder.addRoute(addr)
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            for (addr in config.excludedRoutes) {
                Log.d(TAG, "excludeRoute: $addr")
                vpnBuilder.excludeRoute(addr)
            }
        }

        for (app in config.excludedApplications) {
            Log.d(TAG, "addDisallowedApplication: $app")
            vpnBuilder.addDisallowedApplication(app)
        }

        Log.d(TAG, "setMtu: ${config.mtu}")
        vpnBuilder.setMtu(config.mtu)

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            config.httpProxy?.let {
                Log.d(TAG, "setHttpProxy: $it")
                vpnBuilder.setHttpProxy(it)
            }
        }

        if (config.allowAllAF) {
            Log.d(TAG, "allowFamily")
            vpnBuilder.allowFamily(OsConstants.AF_INET)
            vpnBuilder.allowFamily(OsConstants.AF_INET6)
        }

        Log.d(TAG, "setBlocking: ${config.blockingMode}")
        vpnBuilder.setBlocking(config.blockingMode)
        vpnBuilder.setUnderlyingNetworks(null)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q)
            vpnBuilder.setMetered(false)
    }

    companion object {
        private fun extractLibrary(context: Context, libraryName: String, destination: File): Boolean {
            Log.d(TAG, "Extracting library: $libraryName")
            val apks = hashSetOf<String>()
            context.applicationInfo.run {
                sourceDir?.let { apks += it }
                splitSourceDirs?.let { apks += it }
            }
            for (abi in Build.SUPPORTED_ABIS) {
                for (apk in apks) {
                    ZipFile(File(apk), ZipFile.OPEN_READ).use { zipFile ->
                        val mappedName = System.mapLibraryName(libraryName)
                        val libraryZipPath = listOf("lib", abi, mappedName).joinToString(File.separator)
                        val zipEntry = zipFile.getEntry(libraryZipPath)
                        zipEntry?.let {
                            Log.d(TAG, "Extracting apk:/$libraryZipPath to ${destination.absolutePath}")
                            FileOutputStream(destination).use { outStream ->
                                zipFile.getInputStream(zipEntry).use { inStream ->
                                    inStream.copyTo(outStream, 32 * 1024)
                                    outStream.fd.sync()
                                }
                            }
                        }
                        return true
                    }
                }
            }
            return false
        }

        @SuppressLint("UnsafeDynamicallyLoadedCode")
        fun loadSharedLibrary(context: Context, libraryName: String) {
            Log.d(TAG, "Loading library: $libraryName")
            try {
                System.loadLibrary(libraryName)
                return
            } catch (_: UnsatisfiedLinkError) {
                Log.d(TAG, "Failed to load library, try to extract it from apk")
            }
            var tempFile: File? = null
            try {
                tempFile = File.createTempFile("lib", ".so", context.codeCacheDir)
                if (extractLibrary(context, libraryName, tempFile)) {
                    System.load(tempFile.absolutePath)
                    return
                }
            } catch (e: Exception) {
                throw LoadLibraryException("Failed to load library apk: $libraryName", e)
            } finally {
                tempFile?.delete()
            }
        }
    }
}

private fun VpnService.Builder.addAddress(addr: InetNetwork) = addAddress(addr.address, addr.mask)
private fun VpnService.Builder.addRoute(addr: InetNetwork) = addRoute(addr.address, addr.mask)

@RequiresApi(Build.VERSION_CODES.TIRAMISU)
private fun VpnService.Builder.excludeRoute(addr: InetNetwork) = excludeRoute(IpPrefix(addr.address, addr.mask))
