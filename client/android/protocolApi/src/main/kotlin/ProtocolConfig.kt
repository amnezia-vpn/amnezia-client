package org.amnezia.vpn.protocol

import android.net.InetAddresses
import android.os.Build
import androidx.annotation.RequiresApi
import java.net.InetAddress

data class ProtocolConfig(
    val addresses: Set<InetNetwork>,
    val dnsServers: Set<InetAddress>,
    val routes: Set<InetNetwork>,
    val excludedRoutes: Set<InetNetwork>,
    val excludedApplications: Set<String>,
    val blockingMode: Boolean,
    val mtu: Int
) {

    private constructor(builder: Builder) : this(
        builder.addresses,
        builder.dnsServers,
        builder.routes,
        builder.excludedRoutes,
        builder.excludedApplications,
        builder.blockingMode,
        builder.mtu
    )

    class Builder(blockingMode: Boolean) {
        internal val addresses: MutableSet<InetNetwork> = hashSetOf()
        internal val dnsServers: MutableSet<InetAddress> = hashSetOf()
        internal val routes: MutableSet<InetNetwork> = hashSetOf()
        internal val excludedRoutes: MutableSet<InetNetwork> = hashSetOf()
        internal val excludedApplications: MutableSet<String> = hashSetOf()

        internal var blockingMode: Boolean = blockingMode
            private set

        internal var mtu: Int = 0
            private set

        fun addAddress(addr: InetNetwork) = apply { this.addresses += addr }
        fun addAddresses(addresses: List<InetNetwork>) = apply { this.addresses += addresses }

        fun addDnsServer(dnsServer: InetAddress) = apply { this.dnsServers += dnsServer }
        fun addDnsServers(dnsServers: List<InetAddress>) = apply { this.dnsServers += dnsServers }

        fun addRoute(route: InetNetwork) = apply { this.routes += route }
        fun addRoutes(routes: List<InetNetwork>) = apply { this.routes += routes }

        @RequiresApi(Build.VERSION_CODES.TIRAMISU)
        fun excludeRoute(route: InetNetwork) = apply { this.excludedRoutes += route }
        @RequiresApi(Build.VERSION_CODES.TIRAMISU)
        fun excludeRoutes(routes: List<InetNetwork>) = apply { this.excludedRoutes += routes }

        fun excludeApplication(application: String) = apply { this.excludedApplications += application }
        fun excludeApplications(applications: List<String>) = apply { this.excludedApplications += applications }

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

        fun build(): ProtocolConfig = validate().run { ProtocolConfig(this@Builder) }
    }

    companion object {
        inline fun build(blockingMode: Boolean, block: Builder.() -> Unit): ProtocolConfig =
            Builder(blockingMode).apply(block).build()
    }
}

data class InetNetwork(val address: InetAddress, val mask: Int) {

    override fun toString(): String = "${address.hostAddress}/$mask"

    companion object {
        fun parse(data: String): InetNetwork {
            val split = data.split("/")
            val address = parseInetAddress(split.first())
            val mask = split.last().toInt()
            return InetNetwork(address, mask)
        }
    }
}

data class InetEndpoint(val address: InetAddress, val port: Int) {

    override fun toString(): String = "${address.hostAddress}:$port"

    companion object {
        fun parse(data: String): InetEndpoint {
            val split = data.split(":")
            val address = parseInetAddress(split.first())
            val port = split.last().toInt()
            return InetEndpoint(address, port)
        }
    }
}

fun parseInetAddress(address: String): InetAddress = parseNumericAddressCompat(address)

private val parseNumericAddressCompat: (String) -> InetAddress =
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
        InetAddresses::parseNumericAddress
    } else {
        val m = InetAddress::class.java.getMethod("parseNumericAddress", String::class.java)
        fun(address: String): InetAddress {
            return m.invoke(null, address) as InetAddress
        }
    }
