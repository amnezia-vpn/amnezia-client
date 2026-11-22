package org.amnezia.vpn.protocol

import android.os.Build
import java.net.InetAddress
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.IpRange
import org.amnezia.vpn.util.net.IpRangeSet

open class ProtocolConfig protected constructor(
    val addresses: Set<InetNetwork>,
    val dnsServers: Set<InetAddress>,
    val routes: Set<Route>,
    val includedApplications: Set<String>,
    val excludedApplications: Set<String>,
    val blockingMode: Boolean,
    val mtu: Int
) {

    protected constructor(builder: Builder) : this(
        builder.addresses,
        builder.dnsServers,
        builder.routes,
        builder.includedApplications,
        builder.excludedApplications,
        builder.blockingMode,
        builder.mtu
    )

    open class Builder(blockingMode: Boolean) {
        internal val addresses: MutableSet<InetNetwork> = hashSetOf()
        internal val dnsServers: MutableSet<InetAddress> = hashSetOf()
        internal val routes: MutableSet<Route> = mutableSetOf()
        internal val includedApplications: MutableSet<String> = hashSetOf()
        internal val excludedApplications: MutableSet<String> = hashSetOf()

        internal var blockingMode: Boolean = blockingMode
            private set

        open var mtu: Int = 0
            protected set

        fun addAddress(addr: InetNetwork) = apply { this.addresses += addr }
        fun addAddresses(addresses: Collection<InetNetwork>) = apply { this.addresses += addresses }
        fun clearAddresses() = apply { this.addresses.clear() }

        fun addDnsServer(dnsServer: InetAddress) = apply { this.dnsServers += dnsServer }
        fun addDnsServers(dnsServers: Collection<InetAddress>) = apply { this.dnsServers += dnsServers }

        fun addRoute(route: InetNetwork) = apply { this.routes += Route(route, true) }
        fun addRoutes(routes: Collection<InetNetwork>) = apply { this.routes += routes.map { Route(it, true) } }

        fun excludeRoute(route: InetNetwork) = apply { this.routes += Route(route, false) }
        fun excludeRoutes(routes: Collection<InetNetwork>) = apply { this.routes += routes.map { Route(it, false) } }

        fun clearRoutes() = apply { this.routes.clear() }

        fun prependRoutes(block: Builder.() -> Unit) = apply {
            val savedRoutes = mutableListOf<Route>().apply { addAll(routes) }
            routes.clear()
            block()
            routes.addAll(savedRoutes)
        }

        fun includeApplication(application: String) = apply { this.includedApplications += application }
        fun includeApplications(applications: Collection<String>) = apply { this.includedApplications += applications }

        fun excludeApplication(application: String) = apply { this.excludedApplications += application }
        fun excludeApplications(applications: Collection<String>) = apply { this.excludedApplications += applications }

        fun setBlockingMode(blockingMode: Boolean) = apply { this.blockingMode = blockingMode }

        fun setMtu(mtu: Int) = apply { this.mtu = mtu }

        private fun processRoutes() {
            // replace ::/0 as it may cause LAN connection issues
            val ipv6DefaultRoute = InetNetwork("::", 0)
            if (routes.removeIf { it.include && it.inetNetwork == ipv6DefaultRoute }) {
                prependRoutes {
                    addRoute(InetNetwork("2000::", 3))
                }
            }
            // for older versions of Android, build a list of subnets without excluded routes
            // and add them to routes
            if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU && routes.any { !it.include }) {
                val ipRangeSet = IpRangeSet()
                routes.forEach {
                    if (it.include) ipRangeSet.add(IpRange(it.inetNetwork))
                    else ipRangeSet.remove(IpRange(it.inetNetwork))
                }
                ipRangeSet.remove(IpRange("127.0.0.0", 8))
                ipRangeSet.remove(IpRange("::1", 128))
                routes.clear()
                ipRangeSet.subnets().forEach(::addRoute)
            }
            // filter ipv4 and ipv6 loopback addresses
            val ipv6Loopback = InetNetwork("::1", 128)
            routes.removeIf {
                it.include &&
                    if (it.inetNetwork.isIpv4) it.inetNetwork.address.address[0] == 127.toByte()
                    else it.inetNetwork == ipv6Loopback
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
            processRoutes()
            validate()
        }

        open fun build(): ProtocolConfig = configBuild().run { ProtocolConfig(this@Builder) }
    }

    companion object {
        inline fun build(blockingMode: Boolean, block: Builder.() -> Unit): ProtocolConfig =
            Builder(blockingMode).apply(block).build()
    }
}

data class Route(val inetNetwork: InetNetwork, val include: Boolean)
