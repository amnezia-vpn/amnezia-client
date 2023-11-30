package org.amnezia.vpn.protocol.openvpn

import android.net.ProxyInfo
import android.os.Build
import kotlinx.coroutines.flow.MutableStateFlow
import net.openvpn.ovpn3.ClientAPI_Config
import net.openvpn.ovpn3.ClientAPI_EvalConfig
import net.openvpn.ovpn3.ClientAPI_Event
import net.openvpn.ovpn3.ClientAPI_LogInfo
import net.openvpn.ovpn3.ClientAPI_OpenVPNClient
import net.openvpn.ovpn3.ClientAPI_Status
import net.openvpn.ovpn3.ClientAPI_StringVec
import net.openvpn.ovpn3.ClientAPI_TransportStats
import org.amnezia.vpn.protocol.ProtocolState
import org.amnezia.vpn.protocol.ProtocolState.CONNECTED
import org.amnezia.vpn.protocol.ProtocolState.DISCONNECTED
import org.amnezia.vpn.protocol.VpnStartException
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.net.InetNetwork
import org.amnezia.vpn.util.net.parseInetAddress

private const val TAG = "OpenVpnClient"
private const val EMULATED_EXCLUDE_ROUTES = (1 shl 16)

class OpenVpnClient(
    private val configBuilder: OpenVpnConfig.Builder,
    private val state: MutableStateFlow<ProtocolState>,
    private val getLocalNetworks: (Boolean) -> List<InetNetwork>,
    private val establish: () -> Int,
    private val protect: (Int) -> Boolean
) : ClientAPI_OpenVPNClient() {

    /**************************************************************************
     * Tun builder callbacks
     **************************************************************************/

    // Tun builder methods, loosely based on the Android VpnService.Builder
    // abstraction.  These methods comprise an abstraction layer that
    // allows the OpenVPN C++ core to call out to external methods for
    // establishing the tunnel, adding routes, etc.

    // All methods returning bool use the return
    // value to indicate success (true) or fail (false).
    // tun_builder_new() should be called first, then arbitrary setter methods,
    // and finally tun_builder_establish to return the socket descriptor
    // for the session.  IP addresses are pre-validated before being passed to
    // these methods.
    // This interface is based on Android's VpnService.Builder.

    // Callback to construct a new tun builder
    // Should be called first.
    override fun tun_builder_new(): Boolean {
        Log.v(TAG, "tun_builder_new")
        return true
    }

    // Callback to set MTU of the VPN interface
    // Never called more than once per tun_builder session.
    override fun tun_builder_set_mtu(mtu: Int): Boolean {
        Log.v(TAG, "tun_builder_set_mtu: $mtu")
        configBuilder.setMtu(mtu)
        return true
    }

    // Callback to add network address to VPN interface
    // May be called more than once per tun_builder session
    override fun tun_builder_add_address(
        address: String, prefix_length: Int,
        gateway: String, ipv6: Boolean, net30: Boolean
    ): Boolean {
        Log.v(TAG, "tun_builder_add_address: $address, $prefix_length, $gateway, $ipv6, $net30")
        configBuilder.addAddress(InetNetwork(address, prefix_length))
        return true
    }

    // Callback to add route to VPN interface
    // May be called more than once per tun_builder session
    // metric is optional and should be ignored if < 0
    override fun tun_builder_add_route(address: String, prefix_length: Int, metric: Int, ipv6: Boolean): Boolean {
        Log.v(TAG, "tun_builder_add_route: $address, $prefix_length, $metric, $ipv6")
        if (address == "remote_host") return false
        configBuilder.addRoute(InetNetwork(address, prefix_length))
        return true
    }

    // Callback to exclude route from VPN interface
    // May be called more than once per tun_builder session
    // metric is optional and should be ignored if < 0
    override fun tun_builder_exclude_route(address: String, prefix_length: Int, metric: Int, ipv6: Boolean): Boolean {
        Log.v(TAG, "tun_builder_exclude_route: $address, $prefix_length, $metric, $ipv6")
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            configBuilder.excludeRoute(InetNetwork(address, prefix_length))
        }
        return true
    }

    // Callback to add DNS server to VPN interface
    // May be called more than once per tun_builder session
    // If reroute_dns is true, all DNS traffic should be routed over the
    // tunnel, while if false, only DNS traffic that matches an added search
    // domain should be routed.
    // Guaranteed to be called after tun_builder_reroute_gw.
    override fun tun_builder_add_dns_server(address: String, ipv6: Boolean): Boolean {
        Log.v(TAG, "tun_builder_add_dns_server: $address, $ipv6")
        configBuilder.addDnsServer(parseInetAddress(address))
        return true
    }

    // Optional callback that indicates whether traffic of a certain
    // address family (AF_INET or AF_INET6) should be
    // blocked or allowed, to prevent unencrypted packet leakage when
    // the tunnel is IPv4-only/IPv6-only, but the local machine
    // has connectivity with the other protocol to the internet.
    // Controlled by "block-ipv6" and block-ipv6 config var.
    // If addresses are added for a family this setting should be
    // ignored for that family
    // See also Android's VPNService.Builder.allowFamily method
    /* override fun tun_builder_set_allow_family(af: Int, allow: Boolean): Boolean {
        Log.v(TAG, "tun_builder_set_allow_family: $af, $allow")
        return true
    } */

    // Callback to set address of remote server
    // Never called more than once per tun_builder session.
    override fun tun_builder_set_remote_address(address: String, ipv6: Boolean): Boolean {
        Log.v(TAG, "tun_builder_set_remote_address: $address, $ipv6")
        return true
    }

    // Optional callback that indicates OSI layer, should be 2 or 3.
    // Defaults to 3.
    override fun tun_builder_set_layer(layer: Int): Boolean {
        Log.v(TAG, "tun_builder_set_layer: $layer")
        return layer == 3
    }

    // Callback to set the session name
    // Never called more than once per tun_builder session.
    override fun tun_builder_set_session_name(name: String): Boolean {
        Log.v(TAG, "tun_builder_set_session_name: $name")
        return true
    }

    // Callback to establish the VPN tunnel, returning a file descriptor
    // to the tunnel, which the caller will henceforth own.  Returns -1
    // if the tunnel could not be established.
    // Always called last after tun_builder session has been configured.
    override fun tun_builder_establish(): Int {
        Log.v(TAG, "tun_builder_establish")
        return establish()
    }

    // Callback to reroute default gateway to VPN interface.
    // ipv4 is true if the default route to be added should be IPv4.
    // ipv6 is true if the default route to be added should be IPv6.
    // flags are defined in RGWFlags (rgwflags.hpp).
    // Never called more than once per tun_builder session.
    override fun tun_builder_reroute_gw(ipv4: Boolean, ipv6: Boolean, flags: Long): Boolean {
        Log.v(TAG, "tun_builder_reroute_gw: $ipv4, $ipv6, $flags")
        if ((flags and EMULATED_EXCLUDE_ROUTES.toLong()) != 0L) return true
        if (ipv4) {
            configBuilder.addRoute(InetNetwork("0.0.0.0", 0))
        }
        if (ipv6) {
            configBuilder.addRoute(InetNetwork("::", 0))
        }
        return true
    }

    // Callback to add search domain to DNS resolver
    // May be called more than once per tun_builder session
    // See tun_builder_add_dns_server above for description of
    // reroute_dns parameter.
    // Guaranteed to be called after tun_builder_reroute_gw.
    override fun tun_builder_add_search_domain(domain: String): Boolean {
        Log.v(TAG, "tun_builder_add_search_domain: $domain")
        configBuilder.setSearchDomain(domain)
        return true
    }

    // Callback to set the HTTP proxy
    // Never called more than once per tun_builder session.
    override fun tun_builder_set_proxy_http(host: String, port: Int): Boolean {
        Log.v(TAG, "tun_builder_set_proxy_http: $host, $port")
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            try {
                configBuilder.setHttpProxy(ProxyInfo.buildDirectProxy(host, port))
            } catch (e: Exception) {
                Log.e(TAG, "Could not set proxy: ${e.message}")
                return false
            }
        }
        return true
    }

    // Callback to set the HTTPS proxy
    // Never called more than once per tun_builder session.
    override fun tun_builder_set_proxy_https(host: String, port: Int): Boolean {
        Log.v(TAG, "tun_builder_set_proxy_https: $host, $port")
        return false
    }

    // When the exclude local network option is enabled this
    // function is called to get a list of local networks so routes
    // to exclude them from the VPN network are generated
    // This should be a list of CIDR networks (e.g. 192.168.0.0/24)
    override fun tun_builder_get_local_networks(ipv6: Boolean): ClientAPI_StringVec {
        Log.v(TAG, "tun_builder_get_local_networks: $ipv6")
        val networks = ClientAPI_StringVec()
        for (address in getLocalNetworks(ipv6)) {
            networks.add(address.toString())
        }
        return networks
    }

    // Optional callback to set default value for route metric.
    // Guaranteed to be called before other methods that deal
    // with routes such as tun_builder_add_route and
    // tun_builder_reroute_gw.  Route metric is ignored
    // if < 0.
    /* override fun tun_builder_set_route_metric_default(metric: Int): Boolean {
        Log.v(TAG, "tun_builder_set_route_metric_default: $metric")
        return super.tun_builder_set_route_metric_default(metric)
    } */

    // Callback to add a host which should bypass the proxy
    // May be called more than once per tun_builder session
    /* override fun tun_builder_add_proxy_bypass(bypass_host: String): Boolean {
        Log.v(TAG, "tun_builder_add_proxy_bypass: $bypass_host")
        return super.tun_builder_add_proxy_bypass(bypass_host)
    } */

    // Callback to set the proxy "Auto Config URL"
    // Never called more than once per tun_builder session.
    /* override fun tun_builder_set_proxy_auto_config_url(url: String): Boolean {
        Log.v(TAG, "tun_builder_set_proxy_auto_config_url: $url")
        return super.tun_builder_set_proxy_auto_config_url(url)
    } */

    // Callback to add Windows WINS server to VPN interface.
    // WINS server addresses are always IPv4.
    // May be called more than once per tun_builder session.
    // Guaranteed to be called after tun_builder_reroute_gw.
    /* override fun tun_builder_add_wins_server(address: String): Boolean {
        Log.v(TAG, "tun_builder_add_wins_server: $address")
        return super.tun_builder_add_wins_server(address)
    } */

    // Optional callback to set a DNS suffix on tun/tap adapter.
    // Currently only implemented on Windows, where it will
    // set the "Connection-specific DNS Suffix" property on
    // the TAP driver.
    /* override fun tun_builder_set_adapter_domain_suffix(name: String): Boolean {
        Log.v(TAG, "tun_builder_set_adapter_domain_suffix: $name")
        return super.tun_builder_set_adapter_domain_suffix(name)
    } */

    // Return true if tun interface may be persisted, i.e. rolled
    // into a new session with properties untouched.  This method
    // is only called after all other tests of persistence
    // allowability succeed, therefore it can veto persistence.
    // If persistence is ultimately enabled,
    // tun_builder_establish_lite() will be called.  Otherwise,
    // tun_builder_establish() will be called.
    /* override fun tun_builder_persist(): Boolean {
        Log.v(TAG, "tun_builder_persist")
        return super.tun_builder_persist()
    } */

    // Indicates a reconnection with persisted tun state.
    /* override fun tun_builder_establish_lite() {
        Log.v(TAG, "tun_builder_establish_lite")
        super.tun_builder_establish_lite()
    } */

    // Indicates that tunnel is being torn down.
    // If disconnect == true, then the teardown is occurring
    // prior to final disconnect.
    /* override fun tun_builder_teardown(disconnect: Boolean) {
        Log.v(TAG, "tun_builder_teardown: $disconnect")
        super.tun_builder_teardown(disconnect)
    } */

    /**************************************************************************
     * Connection control methods
     **************************************************************************/

    // Parse OpenVPN configuration file.
    override fun eval_config(arg0: ClientAPI_Config): ClientAPI_EvalConfig {
        Log.v(TAG, "eval_config")
        return super.eval_config(arg0)
    }

    // Primary VPN client connect method, doesn't return until disconnect.
    // Should be called by a worker thread.  This method will make callbacks
    // to event() and log() functions.  Make sure to call eval_config()
    // and possibly provide_creds() as well before this function.
    override fun connect(): ClientAPI_Status {
        Log.v(TAG, "connect")
        return super.connect()
    }

    // Callback to "protect" a socket from being routed through the tunnel.
    // Will be called from the thread executing connect().
    // The remote and ipv6 are the remote host this socket will connect to
    override fun socket_protect(socket: Int, remote: String, ipv6: Boolean): Boolean {
        Log.v(TAG, "socket_protect: $socket, $remote, $ipv6")
        return protect(socket)
    }

    // Stop the client.  Only meaningful when connect() is running.
    // May be called asynchronously from a different thread
    // when connect() is running.
    override fun stop() {
        Log.v(TAG, "stop")
        super.stop()
    }

    // Pause the client -- useful to avoid continuous reconnection attempts
    // when network is down.  May be called from a different thread
    // when connect() is running.
    override fun pause(reason: String) {
        Log.v(TAG, "pause: $reason")
        super.pause(reason)
    }

    // Resume the client after it has been paused.  May be called from a
    // different thread when connect() is running.
    override fun resume() {
        Log.v(TAG, "resume")
        super.resume()
    }

    // Do a disconnect/reconnect cycle n seconds from now.  May be called
    // from a different thread when connect() is running.
    override fun reconnect(seconds: Int) {
        Log.v(TAG, "reconnect")
        super.reconnect(seconds)
    }

    // When a connection is close to timeout, the core will call this
    // method.  If it returns false, the core will disconnect with a
    // CONNECTION_TIMEOUT event.  If true, the core will enter a PAUSE
    // state.
    override fun pause_on_connection_timeout(): Boolean {
        Log.v(TAG, "pause_on_connection_timeout")
        return false
    }

    // Return information about the most recent connection.  Should be called
    // after an event of type "CONNECTED".
    /* override fun connection_info(): ClientAPI_ConnectionInfo {
        Log.v(TAG, "connection_info")
        return super.connection_info()
    } */

    /**************************************************************************
     * Status callbacks
     **************************************************************************/

    // Callback for delivering events during connect() call.
    // Will be called from the thread executing connect().
    override fun event(event: ClientAPI_Event) {
        val name = event.name
        val info = event.info
        Log.v(TAG, "OpenVpn event: $name: $info")
        when (name) {
            "COMPRESSION_ENABLED", "WARN" -> Log.w(TAG, "$name: $info")
            "CONNECTED" -> state.value = CONNECTED
            "DISCONNECTED" -> state.value = DISCONNECTED
            "CONNECTION_TIMEOUT" -> {
                Log.w(TAG, "$name: $info")
                state.value = DISCONNECTED
                // todo: test it
                throw VpnStartException("Connection timeout")
            }
        }
        if (event.error) Log.e(TAG, "OpenVpn ERROR: $name: $info")
        if (event.fatal) Log.e(TAG, "OpenVpn FATAL: $name: $info")
    }

    // Callback for logging.
    // Will be called from the thread executing connect().
    override fun log(arg0: ClientAPI_LogInfo) {
        arg0.text.dropLastWhile { it == '\n' }.let {
            Log.d(TAG, "OpenVpnLog: $it")
        }
    }

    /**************************************************************************
     * Stats methods
     **************************************************************************/

    // return transport stats only
    override fun transport_stats(): ClientAPI_TransportStats {
        Log.v(TAG, "transport_stats")
        return super.transport_stats()
    }

    // return a stats value, index should be >= 0 and < stats_n()
    /* override fun stats_value(index: Int): Long {
        Log.v(TAG, "stats_value: $index")
        return super.stats_value(index)
    } */

    // return all stats in a bundle
    /* override fun stats_bundle(): ClientAPI_LLVector {
        Log.v(TAG, "stats_bundle")
        return super.stats_bundle()
    } */

    // return tun stats only
    /* override fun tun_stats(): ClientAPI_InterfaceStats {
        Log.v(TAG, "tun_stats")
        return super.tun_stats()
    } */

    // post control channel message
    /* override fun post_cc_msg(msg: String) {
        Log.v(TAG, "post_cc_msg: $msg")
        super.post_cc_msg(msg)
    } */
}
