package org.amnezia.vpn.protocol

import android.net.ProxyInfo
import android.os.Build
import androidx.annotation.RequiresApi
import java.net.InetAddress
import org.amnezia.vpn.util.net.InetNetwork

open class ProtocolConfig protected constructor(
    val addresses: Set<InetNetwork>,
    val dnsServers: Set<InetAddress>,
    val searchDomain: String?,
    val routes: Set<InetNetwork>,
    val excludedRoutes: Set<InetNetwork>,
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
        internal val excludedApplications: MutableSet<String> = hashSetOf()

        internal var searchDomain: String? = null
            private set

        internal var httpProxy: ProxyInfo? = null
            private set

        internal var allowAllAF: Boolean = false
            private set

        internal var blockingMode: Boolean = blockingMode
            private set

        open var mtu: Int = 0
            protected set

        fun addAddress(addr: InetNetwork) = apply { this.addresses += addr }
        fun addAddresses(addresses: List<InetNetwork>) = apply { this.addresses += addresses }
        fun clearAddresses() = apply { this.addresses.clear() }

        fun addDnsServer(dnsServer: InetAddress) = apply { this.dnsServers += dnsServer }
        fun addDnsServers(dnsServers: List<InetAddress>) = apply { this.dnsServers += dnsServers }

        fun setSearchDomain(domain: String) = apply { this.searchDomain = domain }

        fun addRoute(route: InetNetwork) = apply { this.routes += route }
        fun addRoutes(routes: List<InetNetwork>) = apply { this.routes += routes }
        fun removeRoute(route: InetNetwork) = apply { this.routes.remove(route) }
        fun clearRoutes() = apply { this.routes.clear() }

        @RequiresApi(Build.VERSION_CODES.TIRAMISU)
        fun excludeRoute(route: InetNetwork) = apply { this.excludedRoutes += route }

        @RequiresApi(Build.VERSION_CODES.TIRAMISU)
        fun excludeRoutes(routes: List<InetNetwork>) = apply { this.excludedRoutes += routes }

        fun excludeApplication(application: String) = apply { this.excludedApplications += application }
        fun excludeApplications(applications: List<String>) = apply { this.excludedApplications += applications }

        @RequiresApi(Build.VERSION_CODES.Q)
        fun setHttpProxy(httpProxy: ProxyInfo) = apply { this.httpProxy = httpProxy }

        fun setAllowAllAF(allowAllAF: Boolean) = apply { this.allowAllAF = allowAllAF }

        fun setBlockingMode(blockingMode: Boolean) = apply { this.blockingMode = blockingMode }

        fun setMtu(mtu: Int) = apply { this.mtu = mtu }

        private fun validate() {
            val errorMessage = StringBuilder()

            with(errorMessage) {
                if (addresses.isEmpty()) appendLine("VPN interface network address not specified.")
                if (routes.isEmpty()) appendLine("VPN interface route not specified.")
                if (mtu == 0) appendLine("MTU not set.")
            }

            if (errorMessage.isNotEmpty()) throw BadConfigException(errorMessage.toString())
        }

        open fun build(): ProtocolConfig = validate().run { ProtocolConfig(this@Builder) }
    }

    companion object {
        inline fun build(blockingMode: Boolean, block: Builder.() -> Unit): ProtocolConfig =
            Builder(blockingMode).apply(block).build()
    }
}
