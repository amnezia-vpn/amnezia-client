package org.amnezia.vpn

/**
 * Android `VpnService` host for the MasterDnsVPN protocol.
 *
 * Inherits the full lifecycle (start, stop, reconnect, foreground
 * notification, split-tunneling glue) from [AmneziaVpnService] and only
 * exists so the platform can resolve a class name in the manifest. The
 * actual MasterDnsVPN engine (SOCKS5 listener, ARQ, resolver pool, DNS
 * tunnel) lives in the `master_dns_vpn` module, owned by the singleton
 * `MasterDnsVpn` that the parent class dispatches to.
 */
class MasterDnsVpnService : AmneziaVpnService()
