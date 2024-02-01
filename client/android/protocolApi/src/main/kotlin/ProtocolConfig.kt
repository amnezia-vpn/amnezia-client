package org.amnezia.vpn.protocol

import android.net.ProxyInfo
import android.os.Build
import androidx.annotation.RequiresApi
import java.net.InetAddress
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.IpRange
import org.amnezia.vpn.util.net.IpRangeSet

open class ProtocolConfig protected constructor(
    val addresses: Set<InetNetwork>,
    val dnsServers: Set<InetAddress>,
    val searchDomain: String?,
    val routes: Set<InetNetwork>,
    val excludedRoutes: Set<InetNetwork>,
    val includedAddresses: Set<InetNetwork>,
    val excludedAddresses: Set<InetNetwork>,
    val excludedApplications: Set<String>,
    val httpProxy: ProxyInfo?,
    val allowAllAF: Boolean,
    val blockingMode: Boolean,
    val mtu: Int
) {

    protected constructor(builder: Builder) : this(
        builder.addresses,
        builder.dnsServers,
        builder.searchDomain,
        builder.routes,
        builder.excludedRoutes,
        builder.includedAddresses,
        builder.excludedAddresses,
        builder.excludedApplications,
        builder.httpProxy,
        builder.allowAllAF,
        builder.blockingMode,
        builder.mtu
    )

    open class Builder(blockingMode: Boolean) {
        internal val addresses: MutableSet<InetNetwork> = hashSetOf()
        internal val dnsServers: MutableSet<InetAddress> = hashSetOf()
        internal val routes: MutableSet<InetNetwork> = hashSetOf()
        internal val excludedRoutes: MutableSet<InetNetwork> = hashSetOf()
        internal val includedAddresses: MutableSet<InetNetwork> = hashSetOf()
        internal val excludedAddresses: MutableSet<InetNetwork> = hashSetOf()
        internal val excludedApplications: MutableSet<String> = hashSetOf()

        internal var searchDomain: String? = null
            private set

        internal var httpProxy: ProxyInfo? = null
            private set

        internal var allowAllAF: Boolean = false
            private set

        internal var blockingMode: Boolean = blockingMode
            private set

        internal var allowSplitTunneling: Boolean = true
            private set

        open var mtu: Int = 0
            protected set

        fun addAddress(addr: InetNetwork) = apply { this.addresses += addr }
        fun addAddresses(addresses: Collection<InetNetwork>) = apply { this.addresses += addresses }
        fun clearAddresses() = apply { this.addresses.clear() }

        fun addDnsServer(dnsServer: InetAddress) = apply { this.dnsServers += dnsServer }
        fun addDnsServers(dnsServers: Collection<InetAddress>) = apply { this.dnsServers += dnsServers }

        fun setSearchDomain(domain: String) = apply { this.searchDomain = domain }

        fun addRoute(route: InetNetwork) = apply { this.routes += route }
        fun addRoutes(routes: Collection<InetNetwork>) = apply { this.routes += routes }
        fun removeRoute(route: InetNetwork) = apply { this.routes.remove(route) }
        fun clearRoutes() = apply { this.routes.clear() }

        fun excludeRoute(route: InetNetwork) = apply { this.excludedRoutes += route }
        fun excludeRoutes(routes: Collection<InetNetwork>) = apply { this.excludedRoutes += routes }

        fun includeAddress(addr: InetNetwork) = apply { this.includedAddresses += addr }
        fun includeAddresses(addresses: Collection<InetNetwork>) = apply { this.includedAddresses += addresses }

        fun excludeAddress(addr: InetNetwork) = apply { this.excludedAddresses += addr }
        fun excludeAddresses(addresses: Collection<InetNetwork>) = apply { this.excludedAddresses += addresses }

        fun excludeApplication(application: String) = apply { this.excludedApplications += application }
        fun excludeApplications(applications: Collection<String>) = apply { this.excludedApplications += applications }

        @RequiresApi(Build.VERSION_CODES.Q)
        fun setHttpProxy(httpProxy: ProxyInfo) = apply { this.httpProxy = httpProxy }

        fun setAllowAllAF(allowAllAF: Boolean) = apply { this.allowAllAF = allowAllAF }

        fun setBlockingMode(blockingMode: Boolean) = apply { this.blockingMode = blockingMode }

        fun disableSplitTunneling() = apply { this.allowSplitTunneling = false }

        fun setMtu(mtu: Int) = apply { this.mtu = mtu }

        private fun processSplitTunneling() {
            if (includedAddresses.isNotEmpty() && excludedAddresses.isNotEmpty()) {
                throw BadConfigException("Config contains addresses for inclusive and exclusive split tunneling at the same time")
            }

            if (includedAddresses.isNotEmpty()) {
                // remove default routes, if any
                removeRoute(InetNetwork("0.0.0.0", 0))
                removeRoute(InetNetwork("::", 0))
                if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
                    // for older versions of Android, add the default route to the excluded routes
                    // to correctly build the excluded subnets list later
                    excludeRoute(InetNetwork("0.0.0.0", 0))
                }
                addRoutes(includedAddresses)
            } else if (excludedAddresses.isNotEmpty()) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    // default routes are required for split tunneling in newer versions of Android
                    addRoute(InetNetwork("0.0.0.0", 0))
                    addRoute(InetNetwork("::", 0))
                }
                excludeRoutes(excludedAddresses)
            }
        }

        private fun processExcludedRoutes() {
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU && excludedRoutes.isNotEmpty()) {
                // todo: rewrite, taking into account the current routes
                // for older versions of Android, build a list of subnets without excluded routes
                // and add them to routes
                val ipRangeSet = IpRangeSet()
                ipRangeSet.remove(IpRange("127.0.0.0", 8))
                excludedRoutes.forEach {
                    ipRangeSet.remove(IpRange(it))
                }
                // remove default routes, if any
                removeRoute(InetNetwork("0.0.0.0", 0))
                removeRoute(InetNetwork("::", 0))
                ipRangeSet.subnets().forEach(::addRoute)
                addRoute(InetNetwork("2000::", 3))
            }
        }

        private fun validate() {
            val errorMessage = StringBuilder()

            with(errorMessage) {
                if (addresses.isEmpty()) appendLine("VPN interface network address not specified.")
                if (routes.isEmpty()) appendLine("VPN interface route not specified.")
                if (mtu == 0) appendLine("MTU not set.")
            }

            if (errorMessage.isNotEmpty()) throw BadConfigException(errorMessage.toString())
        }

        protected fun configBuild() {
            processSplitTunneling()
            processExcludedRoutes()
            validate()
        }

        open fun build(): ProtocolConfig = configBuild().run { ProtocolConfig(this@Builder) }
    }

    companion object {
        inline fun build(blockingMode: Boolean, block: Builder.() -> Unit): ProtocolConfig =
            Builder(blockingMode).apply(block).build()
    }
}
