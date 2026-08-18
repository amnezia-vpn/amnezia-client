import Darwin
import Foundation
import NetworkExtension

enum XrayErrors: Error {
    case noXrayConfig
    case xrayConfigIsWrong
    case cantSaveXrayConfig
    case cantParseListenAndPort
    case cantAcquireLocalPort
    case cantSaveHevSocksConfig
}

extension Constants {
    static let cachesDirectory: URL = {
        if let cachesDirectoryURL = FileManager.default.urls(for: .cachesDirectory,
                                                             in: .userDomainMask).first {
            return cachesDirectoryURL
        } else {
            fatalError("Unable to retrieve caches directory.")
        }
    }()
}

extension PacketTunnelProvider {
    /// TCP port chosen by the OS on IPv6 loopback (::1), matching inbound listen address.
    private func acquireFreeLocalPort() throws -> Int {
        let fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)
        guard fd != -1 else {
            throw XrayErrors.cantAcquireLocalPort
        }
        defer { close(fd) }
        var reuse: Int32 = 1
        _ = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, socklen_t(MemoryLayout<Int32>.size))
        var addr = sockaddr_in6()
        addr.sin6_len = UInt8(MemoryLayout<sockaddr_in6>.size)
        addr.sin6_family = sa_family_t(AF_INET6)
        addr.sin6_port = in_port_t(0).bigEndian
        addr.sin6_addr = in6addr_loopback
        addr.sin6_scope_id = 0
        let bindResult = withUnsafePointer(to: &addr) { ptr in
            ptr.withMemoryRebound(to: sockaddr.self, capacity: 1) { p in
                bind(fd, p, socklen_t(MemoryLayout<sockaddr_in6>.size))
            }
        }
        guard bindResult == 0 else {
            throw XrayErrors.cantAcquireLocalPort
        }
        var bound = sockaddr_in6()
        var len = socklen_t(MemoryLayout<sockaddr_in6>.size)
        let gr = withUnsafeMutablePointer(to: &bound) { p in
            p.withMemoryRebound(to: sockaddr.self, capacity: 1) { bp in
                getsockname(fd, bp, &len)
            }
        }
        guard gr == 0 else {
            throw XrayErrors.cantAcquireLocalPort
        }
        return Int(bound.sin6_port.byteSwapped)
    }

    private func applyXraySplitTunnel(_ xrayConfig: XrayConfig,
                                      settings: NEPacketTunnelNetworkSettings) {
        guard let splitTunnelType = xrayConfig.splitTunnelType else {
            return
        }

        guard let splitTunnelSites = xrayConfig.splitTunnelSites else {
            xrayLog(.error, message: "Split tunnel sites are not set")
            return
        }

        if splitTunnelType == 1 {
            var ipv4IncludedRoutes = [NEIPv4Route]()

            for allowedIPString in splitTunnelSites {
                if let allowedIP = IPAddressRange(from: allowedIPString) {
                    ipv4IncludedRoutes.append(NEIPv4Route(
                        destinationAddress: "\(allowedIP.address)",
                        subnetMask: "\(allowedIP.subnetMask())"))
                }
            }

            settings.ipv4Settings?.includedRoutes = ipv4IncludedRoutes
        } else if splitTunnelType == 2 {
            var ipv4ExcludedRoutes = [NEIPv4Route]()

            for excludedIPString in splitTunnelSites {
                if let excludedIP = IPAddressRange(from: excludedIPString) {
                    ipv4ExcludedRoutes.append(NEIPv4Route(
                        destinationAddress: "\(excludedIP.address)",
                        subnetMask: "\(excludedIP.subnetMask())"))
                }
            }

            settings.ipv4Settings?.excludedRoutes = ipv4ExcludedRoutes
        }
    }

    func startXray(completionHandler: @escaping (Error?) -> Void) {

        // Xray configuration
        guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
              let providerConfiguration = protocolConfiguration.providerConfiguration,
              let configData = providerConfiguration[Constants.xrayConfigKey] as? Data else {
            xrayLog(.error, message: "Can't get xray configuration")
            completionHandler(XrayErrors.noXrayConfig)
            return
        }

        // Tunnel settings
        let ipv6Enabled = false
        let hideVPNIcon = false

        let settings = NEPacketTunnelNetworkSettings(tunnelRemoteAddress: "254.1.1.1")
        settings.mtu = 9000

        settings.ipv4Settings = {
            let settings = NEIPv4Settings(addresses: ["198.18.0.1"], subnetMasks: ["255.255.0.0"])
            settings.includedRoutes = [NEIPv4Route.default()]
            return settings
        }()

        settings.ipv6Settings = {
            guard ipv6Enabled else {
                return nil
            }
            let settings = NEIPv6Settings(addresses: ["fd6e:a81b:704f:1211::1"], networkPrefixLengths: [64])
            settings.includedRoutes = [NEIPv6Route.default()]
            if hideVPNIcon {
                settings.excludedRoutes = [NEIPv6Route(destinationAddress: "::", networkPrefixLength: 128)]
            }
            return settings
        }()

        do {
            let xrayConfig = try JSONDecoder().decode(XrayConfig.self,
                                                      from: configData)

            var dnsArray = [String]()
            if let dns1 = xrayConfig.dns1 {
                dnsArray.append(dns1)
            }
            if let dns2 = xrayConfig.dns2 {
                dnsArray.append(dns2)
            }

            settings.dnsSettings = !dnsArray.isEmpty
            ? NEDNSSettings(servers: dnsArray)
            : NEDNSSettings(servers: ["1.1.1.1"])
            applyXraySplitTunnel(xrayConfig, settings: settings)

            let xrayConfigData = xrayConfig.config.data(using: .utf8)

            guard let xrayConfigData else {
                xrayLog(.error, message: "Can't encode config to data")
                completionHandler(XrayErrors.xrayConfigIsWrong)
                return
            }

            let jsonDict = try JSONSerialization.jsonObject(with: xrayConfigData,
                                                            options: []) as? [String: Any]

            guard var jsonDict else {
                xrayLog(.error, message: "Can't parse address and port for hevSocks")
                completionHandler(XrayErrors.cantParseListenAndPort)
                return
            }

            let port = try acquireFreeLocalPort()
            let address = "::1"

            // Extract existing SOCKS5 credentials or generate new ones per session.
            let socksCredentials = ensureInboundAuth(jsonDict: &jsonDict, port: port, address: address)

            let updatedData = try JSONSerialization.data(withJSONObject: jsonDict, options: [])

            setTunnelNetworkSettings(settings) { [weak self] error in
                if let error {
                    completionHandler(error)
                    return
                }

                self?.updateActiveInterfaceIndexForCurrentPath()

                // Launch xray
                self?.setupAndStartXray(configData: updatedData) { xrayError in
                    if let xrayError {
                        completionHandler(xrayError)
                        return
                    }

                    // Launch hevSocks
                    self?.setupAndRunTun2socks(configData: updatedData,
                                               address: address,
                                               port: port,
                                               username: socksCredentials.username,
                                               password: socksCredentials.password,
                                               completionHandler: completionHandler)
                }
            }
        } catch {
            completionHandler(error)
            return
        }
    }

    func stopXray(completionHandler: () -> Void) {
        Socks5Tunnel.quit()
        LibXrayStopXray()
        completionHandler()
    }

    func sockCallback(fd: uintptr_t) {
        if activeIfaceIdx != 0 {
            withUnsafePointer(to: activeIfaceIdx) { ptr in
                setsockopt(Int32(fd), IPPROTO_IP, IP_BOUND_IF, ptr, socklen_t(MemoryLayout<UInt32>.size))
                setsockopt(Int32(fd), IPPROTO_IPV6, IPV6_BOUND_IF, ptr, socklen_t(MemoryLayout<UInt32>.size))
            }
        }
    }

    private struct SocksCredentials {
        let username: String
        let password: String
    }

    private func indexOfSocksInbound(in inboundsArray: [[String: Any]]) -> Int? {
        for (i, inbound) in inboundsArray.enumerated() {
            guard let proto = inbound["protocol"] as? String else { continue }
            if proto.caseInsensitiveCompare("socks") == .orderedSame {
                return i
            }
        }
        return nil
    }

    // Returns existing SOCKS5 credentials from the inbound config, or generates and injects
    // new random ones. Also sets port and address on the socks inbound entry.
    private func ensureInboundAuth(jsonDict: inout [String: Any], port: Int, address: String) -> SocksCredentials {
        var inboundsArray = jsonDict["inbounds"] as? [[String: Any]] ?? []

        if let socksIdx = indexOfSocksInbound(in: inboundsArray) {
            var inbound = inboundsArray[socksIdx]
            inbound["port"] = port
            inbound["listen"] = address

            var settings = inbound["settings"] as? [String: Any] ?? [:]
            if let accounts = settings["accounts"] as? [[String: Any]],
               let first = accounts.first,
               let user = first["user"] as? String, !user.isEmpty,
               let pass = first["pass"] as? String, !pass.isEmpty {
                // Re-use existing credentials, but always enforce auth mode in case the
                // imported config had accounts but auth: "noauth" (or no auth field).
                settings["auth"] = "password"
                inbound["settings"] = settings
                inboundsArray[socksIdx] = inbound
                jsonDict["inbounds"] = inboundsArray
                return SocksCredentials(username: user, password: pass)
            }

            // Generate new random credentials for this session
            let user = UUID().uuidString.replacingOccurrences(of: "-", with: "").lowercased().prefix(16)
            let pass = UUID().uuidString.replacingOccurrences(of: "-", with: "").lowercased()
            settings["auth"] = "password"
            settings["accounts"] = [["user": String(user), "pass": pass]]
            inbound["settings"] = settings
            inboundsArray[socksIdx] = inbound
            jsonDict["inbounds"] = inboundsArray
            return SocksCredentials(username: String(user), password: pass)
        }

        // Fallback: no socks inbound — generate credentials but can't inject
        let user = UUID().uuidString.replacingOccurrences(of: "-", with: "").lowercased().prefix(16)
        let pass = UUID().uuidString.replacingOccurrences(of: "-", with: "").lowercased()
        return SocksCredentials(username: String(user), password: pass)
    }

    private func setupAndStartXray(configData: Data,
                                   completionHandler: @escaping (Error?) -> Void) {
        let path = Constants.cachesDirectory.appendingPathComponent("config.json", isDirectory: false).path
        guard FileManager.default.createFile(atPath: path, contents: configData) else {
            xrayLog(.error, message: "Can't save xray configuration")
            completionHandler(XrayErrors.cantSaveXrayConfig)
            return
        }

        updateActiveInterfaceIndexForCurrentPath()

        let ctx = Unmanaged.passUnretained(self).toOpaque()
        let cb: libxray_sockcallback = { (fd, ctx) in
            guard let ctx = ctx else { return }
            let instance = Unmanaged<PacketTunnelProvider>.fromOpaque(ctx).takeUnretainedValue()

            instance.sockCallback(fd: fd)
        }
        LibXraySetSockCallback(cb, ctx)

        LibXrayRunXray(nil,
                       path,
                       Int64.max)

        completionHandler(nil)
        xrayLog(.info, message: "Xray started")
    }

    private func setupAndRunTun2socks(configData: Data,
                                      address: String,
                                      port: Int,
                                      username: String,
                                      password: String,
                                      completionHandler: @escaping (Error?) -> Void) {
        let config = """
        tunnel:
          mtu: 9000
        socks5:
          port: \(port)
          address: \(address)
          username: \(username)
          password: \(password)
          udp: 'udp'
        misc:
          task-stack-size: 20480
          connect-timeout: 5000
          read-write-timeout: 60000
          log-file: stderr
          log-level: error
          limit-nofile: 65535
        """

        let configurationFilePath = Constants.cachesDirectory.appendingPathComponent("config.yml", isDirectory: false).path
        guard FileManager.default.createFile(atPath: configurationFilePath, contents: config.data(using: .utf8)!) else {
            xrayLog(.info, message: "Cant save hevSocks configuration")
            completionHandler(XrayErrors.cantSaveHevSocksConfig)
            return
        }

        DispatchQueue.global().async {
            xrayLog(.info, message: "Hev socks started")
            completionHandler(nil)
            Socks5Tunnel.run(withConfig: configurationFilePath)
        }
    }
}
