import Darwin
import Foundation
import Network
import NetworkExtension
import os
import Xtun

struct Constants {
    static let xtunConfigKey = "xtun"
    static let kActionStatus = "status"
    static let kMessageKeyAction = "action"
}

private enum SplitTunnelType: Int {
    case disabled = 0
    case include = 1
    case exclude = 2
}

private enum TunnelStatus: Int {
    case disconnected = 0
    case connected = 1
    case noNetwork = 2
}

private struct TunnelSettings: Decodable {
    let proto: String
    let config: String
    let dns: [String]
    let splitTunnelType: Int
    let splitTunnelSites: [String]
    let mtu: Int
}

private struct TunnelError: LocalizedError {
    let errorDescription: String?

    init(_ message: String) {
        errorDescription = message
    }
}

class PacketTunnelProvider: NEPacketTunnelProvider {
    private static let interfaceAddress4 = "172.19.0.1"
    private static let interfaceMask4 = "255.255.255.252"
    private static let interfaceAddress6 = "fdfe:dcba:9876::1"
    private static let interfacePrefix6 = 126
    private static let defaultMtu = 1280

    private let queue = DispatchQueue(label: "org.amnezia.xtun")
    private let pathMonitor = NWPathMonitor()
    private var proxy: XtunProxy?
    private var tunnel: XtunTunnel?
    private var status = TunnelStatus.disconnected
    private var uplink: (interface: NWInterface?, gateways: [Network.NWEndpoint])?

    override func startTunnel(options: [String: NSObject]? = nil,
                              completionHandler: @escaping ((any Error)?) -> Void) {
        neLog(.info, message: "Start tunnel")

        let settings: TunnelSettings
        do {
            settings = try loadSettings()
        } catch {
            neLog(.error, message: "Failed to read the tunnel settings: \(error.localizedDescription)")
            completionHandler(error)
            return
        }

        let proto: XtunProxy.Proto
        switch settings.proto {
        case "awg": proto = .awg
        case "xray": proto = .xray
        default:
            completionHandler(TunnelError("The protocol is not supported by xtun"))
            return
        }

        queue.async {
            do {
                let proxy = try XtunProxy(proto: proto,
                                          config: settings.config,
                                          onLog: { neLog(.info, title: "xtun", message: $0) },
                                          onStatus: { [weak self] status in self?.queue.async { self?.update(status) } })
                let tunnel = try XtunTunnel(ifAddrs: ["\(Self.interfaceAddress4)/30",
                                                      "\(Self.interfaceAddress6)/\(Self.interfacePrefix6)"]) { [weak self] dns, routes, mtu in
                    guard let self else { throw TunnelError("The tunnel provider is gone") }
                    return try self.createTun(settings: settings, dns: dns, routes: routes, mtu: mtu)
                }
                try tunnel.attach(proxy)
                try tunnel.start()

                self.proxy = proxy
                self.tunnel = tunnel
                self.pathMonitor.pathUpdateHandler = { [weak self] path in self?.pathChanged(path) }
                self.pathMonitor.start(queue: self.queue)
                completionHandler(nil)
            } catch {
                neLog(.error, message: "Failed to start xtun: \(error.localizedDescription)")
                completionHandler(error)
            }
        }
    }

    override func stopTunnel(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        neLog(.info, message: "Stopping tunnel: reason: \(reason.amneziaDescription)")
        queue.async {
            self.pathMonitor.cancel()
            self.uplink = nil
            if let proxy = self.proxy, let tunnel = self.tunnel {
                tunnel.detach(proxy)
            }
            self.tunnel = nil
            self.proxy = nil
            self.status = .disconnected
            completionHandler()
        }
    }

    override func handleAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let completionHandler else { return }

        guard let message = try? JSONSerialization.jsonObject(with: messageData) as? [String: Any],
              message[Constants.kMessageKeyAction] as? String == Constants.kActionStatus else {
            completionHandler(nil)
            return
        }

        queue.async {
            completionHandler(try? JSONSerialization.data(withJSONObject: ["status": self.status.rawValue]))
        }
    }

    private func loadSettings() throws -> TunnelSettings {
        guard let protocolConfiguration = protocolConfiguration as? NETunnelProviderProtocol,
              let data = protocolConfiguration.providerConfiguration?[Constants.xtunConfigKey] as? Data else {
            throw TunnelError("The tunnel settings are missing")
        }
        return try JSONDecoder().decode(TunnelSettings.self, from: data)
    }

    private func update(_ xtunStatus: XtunProxy.Status) {
        neLog(.info, message: "xtun status: \(String(describing: xtunStatus))")
        switch xtunStatus {
        case .connected: status = .connected
        case .noNetwork: status = .noNetwork
        default: status = .disconnected
        }
    }

    private func pathChanged(_ path: Network.NWPath) {
        guard let tunnel, path.status == .satisfied else { return }
        let current = (interface: path.availableInterfaces.first, gateways: path.gateways)
        if let uplink, uplink != current {
            neLog(.info, message: "Network changed")
            tunnel.networkChanged()
        }
        uplink = current
    }

    private func createTun(settings: TunnelSettings, dns: [String], routes: [String], mtu: Int32) throws -> Int32 {
        let splitTunnelType = SplitTunnelType(rawValue: settings.splitTunnelType) ?? .disabled
        let includedPrefixes = splitTunnelType == .include ? settings.splitTunnelSites : routes
        let excludedPrefixes = splitTunnelType == .exclude ? settings.splitTunnelSites : []
        let (includedRoutes4, includedRoutes6) = try Self.parseRoutes(includedPrefixes)
        let (excludedRoutes4, excludedRoutes6) = try Self.parseRoutes(excludedPrefixes)

        let ipv4Settings = NEIPv4Settings(addresses: [Self.interfaceAddress4], subnetMasks: [Self.interfaceMask4])
        ipv4Settings.includedRoutes = includedRoutes4
        ipv4Settings.excludedRoutes = excludedRoutes4

        let ipv6Settings = NEIPv6Settings(addresses: [Self.interfaceAddress6],
                                          networkPrefixLengths: [NSNumber(value: Self.interfacePrefix6)])
        ipv6Settings.includedRoutes = includedRoutes6
        ipv6Settings.excludedRoutes = excludedRoutes6

        let dnsSettings = NEDNSSettings(servers: dns.isEmpty ? settings.dns : dns)
        dnsSettings.matchDomains = [""]

        let networkSettings = NEPacketTunnelNetworkSettings(tunnelRemoteAddress: protocolConfiguration.serverAddress ?? "127.0.0.1")
        networkSettings.ipv4Settings = ipv4Settings
        networkSettings.ipv6Settings = ipv6Settings
        networkSettings.dnsSettings = dnsSettings
        networkSettings.mtu = NSNumber(value: mtu > 0 ? Int(mtu) : (settings.mtu > 0 ? settings.mtu : Self.defaultMtu))

        let done = DispatchSemaphore(value: 0)
        var settingsError: Error?
        setTunnelNetworkSettings(networkSettings) { error in
            settingsError = error
            done.signal()
        }
        done.wait()

        if let settingsError {
            throw settingsError
        }

        guard let fd = Self.utunFileDescriptor() else {
            throw TunnelError("Failed to find the utun file descriptor")
        }
        return fd
    }

    private static func utunFileDescriptor() -> Int32? {
        let SYSPROTO_CONTROL: Int32 = 2
        let UTUN_OPT_IFNAME: Int32 = 2
        var name = [CChar](repeating: 0, count: Int(IFNAMSIZ))
        for fd: Int32 in 0...1024 {
            var length = socklen_t(name.count)
            if getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, &name, &length) == 0,
               String(cString: name).hasPrefix("utun") {
                return fd
            }
        }
        return nil
    }

    private static func parseRoutes(_ prefixes: [String]) throws -> ([NEIPv4Route], [NEIPv6Route]) {
        var routes4: [NEIPv4Route] = []
        var routes6: [NEIPv6Route] = []
        for prefix in prefixes {
            let parts = prefix.split(separator: "/", maxSplits: 1)
            let address = String(parts[0])
            let isIpv6 = address.contains(":")
            guard let length = parts.count == 2 ? Int(parts[1]) : (isIpv6 ? 128 : 32) else {
                throw TunnelError("Invalid route \(prefix)")
            }
            if isIpv6 {
                routes6.append(NEIPv6Route(destinationAddress: address, networkPrefixLength: NSNumber(value: length)))
            } else {
                routes4.append(NEIPv4Route(destinationAddress: address, subnetMask: ipv4Mask(length)))
            }
        }
        return (routes4, routes6)
    }

    private static func ipv4Mask(_ length: Int) -> String {
        let mask: UInt32 = length <= 0 ? 0 : ~UInt32(0) << UInt32(32 - min(length, 32))
        return [24, 16, 8, 0].map { String((mask >> UInt32($0)) & 0xff) }.joined(separator: ".")
    }
}

extension NEProviderStopReason {
  var amneziaDescription: String {
    switch self {
    case .none:
      return "No specific reason"
    case .userInitiated:
      return "The user stopped the NE"
    case .providerFailed:
      return "The NE failed to function correctly"
    case .noNetworkAvailable:
      return "No network connectivity is currently available"
    case .unrecoverableNetworkChange:
      return "The device’s network connectivity changed"
    case .providerDisabled:
      return "The NE was disabled"
    case .authenticationCanceled:
      return "The authentication process was canceled"
    case .configurationFailed:
      return "The VPNC is invalid"
    case .idleTimeout:
      return "The session timed out"
    case .configurationDisabled:
      return "The VPNC was disabled"
    case .configurationRemoved:
      return "The VPNC was removed"
    case .superceded:
      return "VPNC was superceded by a higher-priority VPNC"
    case .userLogout:
      return "The user logged out"
    case .userSwitch:
      return "The current console user changed"
    case .connectionFailed:
      return "The connection failed"
    case .internalError:
      return "The network extension reported an internal error"
    case .sleep:
      return "A stop reason indicating the VPNC enabled disconnect on sleep and the device went to sleep"
    case .appUpdate:
      return "appUpdat"
    @unknown default:
      return "@unknown default"
    }
  }
}
