import Foundation
import NetworkExtension

enum Hysteria2Errors: Error {
    case noConfig
    case invalidConfig
    case cantSaveConfig
    case cantSaveHevSocksConfig
}

extension Constants {
    static let hysteria2ConfigKey = "hysteria2"
}

extension PacketTunnelProvider {

    func startHysteria2(completionHandler: @escaping (Error?) -> Void) {
        guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
              let providerConfiguration = protocolConfiguration.providerConfiguration,
              let configData = providerConfiguration[Constants.hysteria2ConfigKey] as? Data else {
            hysteria2Log(.error, message: "Can't get Hysteria2 configuration")
            completionHandler(Hysteria2Errors.noConfig)
            return
        }

        let ipv6Enabled = false

        let settings = NEPacketTunnelNetworkSettings(tunnelRemoteAddress: "254.1.1.1")
        settings.mtu = 9000

        settings.ipv4Settings = {
            let s = NEIPv4Settings(addresses: ["198.18.0.1"], subnetMasks: ["255.255.0.0"])
            s.includedRoutes = [NEIPv4Route.default()]
            return s
        }()

        settings.ipv6Settings = ipv6Enabled ? {
            let s = NEIPv6Settings(addresses: ["fd6e:a81b:704f:1211::1"], networkPrefixLengths: [64])
            s.includedRoutes = [NEIPv6Route.default()]
            return s
        }() : nil

        do {
            let config = try JSONDecoder().decode(Hysteria2Config.self, from: configData)

            var dnsServers = [String]()
            if let dns1 = config.dns1, !dns1.isEmpty { dnsServers.append(dns1) }
            if let dns2 = config.dns2, !dns2.isEmpty { dnsServers.append(dns2) }
            settings.dnsSettings = NEDNSSettings(servers: dnsServers.isEmpty ? ["1.1.1.1"] : dnsServers)

            let socksAddress = "::1"
            let socksPort = 10808

            // Generate Hysteria2 YAML client config
            guard let yamlData = buildHysteria2Config(config,
                                                      socksAddress: socksAddress,
                                                      socksPort: socksPort) else {
                hysteria2Log(.error, message: "Failed to build Hysteria2 config")
                completionHandler(Hysteria2Errors.invalidConfig)
                return
            }

            setTunnelNetworkSettings(settings) { [weak self] error in
                if let error {
                    completionHandler(error)
                    return
                }

                self?.updateActiveInterfaceIndexForCurrentPath()

                self?.setupAndStartHysteria2(yamlData: yamlData) { hysteria2Error in
                    if let hysteria2Error {
                        completionHandler(hysteria2Error)
                        return
                    }

                    self?.setupAndRunTun2socksForHysteria2(address: socksAddress,
                                                           port: socksPort,
                                                           completionHandler: completionHandler)
                }
            }
        } catch {
            hysteria2Log(.error, message: "Failed to decode Hysteria2 config: \(error)")
            completionHandler(error)
        }
    }

    func stopHysteria2(completionHandler: () -> Void) {
        Socks5Tunnel.quit()
        LibHysteria2StopClient()
        completionHandler()
    }

    func hysteria2SockCallback(fd: uintptr_t) {
        if activeIfaceIdx != 0 {
            withUnsafePointer(to: activeIfaceIdx) { ptr in
                setsockopt(Int32(fd), IPPROTO_IP, IP_BOUND_IF, ptr, socklen_t(MemoryLayout<UInt32>.size))
                setsockopt(Int32(fd), IPPROTO_IPV6, IPV6_BOUND_IF, ptr, socklen_t(MemoryLayout<UInt32>.size))
            }
        }
    }

    // MARK: - Private helpers

    private func buildHysteria2Config(_ config: Hysteria2Config,
                                      socksAddress: String,
                                      socksPort: Int) -> Data? {
        let serverAddr = "\(config.server):\(config.port)"
        let sni = config.sni ?? config.server
        let insecure = config.insecure ?? false
        let upMbps = config.upMbps ?? 20
        let downMbps = config.downMbps ?? 100

        var yaml = """
        server: \(serverAddr)
        auth: \(config.password)
        bandwidth:
          up: \(upMbps) mbps
          down: \(downMbps) mbps
        tls:
          sni: \(sni)
          insecure: \(insecure)
        socks5:
          listen: "[\(socksAddress)]:\(socksPort)"
        """

        if let obfsType = config.obfs, !obfsType.isEmpty,
           let obfsPassword = config.obfsPassword, !obfsPassword.isEmpty {
            yaml += """

            obfs:
              type: \(obfsType)
              \(obfsType):
                password: \(obfsPassword)
            """
        }

        return yaml.data(using: .utf8)
    }

    private func setupAndStartHysteria2(yamlData: Data,
                                        completionHandler: @escaping (Error?) -> Void) {
        let configPath = Constants.cachesDirectory
            .appendingPathComponent("hysteria2-config.yaml", isDirectory: false).path

        guard FileManager.default.createFile(atPath: configPath, contents: yamlData) else {
            hysteria2Log(.error, message: "Can't save Hysteria2 configuration")
            completionHandler(Hysteria2Errors.cantSaveConfig)
            return
        }

        updateActiveInterfaceIndexForCurrentPath()

        // Register socket callback so QUIC UDP sockets get bound to the active interface
        let ctx = Unmanaged.passUnretained(self).toOpaque()
        let cb: libhysteria2_sockcallback = { (fd, ctx) in
            guard let ctx = ctx else { return }
            let instance = Unmanaged<PacketTunnelProvider>.fromOpaque(ctx).takeUnretainedValue()
            instance.hysteria2SockCallback(fd: fd)
        }
        LibHysteria2SetSockCallback(cb, ctx)

        // Run Hysteria2 client in background — it blocks until stopped
        DispatchQueue.global().async {
            hysteria2Log(.info, message: "Hysteria2 client starting")
            LibHysteria2RunClient(configPath)
            hysteria2Log(.info, message: "Hysteria2 client exited")
        }

        // Give the client a moment to bind before we connect tun2socks
        DispatchQueue.global().asyncAfter(deadline: .now() + 0.3) {
            completionHandler(nil)
        }
    }

    private func setupAndRunTun2socksForHysteria2(address: String,
                                                   port: Int,
                                                   completionHandler: @escaping (Error?) -> Void) {
        let config = """
        tunnel:
          mtu: 9000
        socks5:
          port: \(port)
          address: \(address)
          udp: 'udp'
        misc:
          task-stack-size: 20480
          connect-timeout: 5000
          read-write-timeout: 60000
          log-file: stderr
          log-level: error
          limit-nofile: 65535
        """

        let configPath = Constants.cachesDirectory
            .appendingPathComponent("hysteria2-hev.yml", isDirectory: false).path

        guard FileManager.default.createFile(atPath: configPath, contents: config.data(using: .utf8)!) else {
            hysteria2Log(.error, message: "Can't save hev-socks5-tunnel configuration for Hysteria2")
            completionHandler(Hysteria2Errors.cantSaveHevSocksConfig)
            return
        }

        DispatchQueue.global().async {
            hysteria2Log(.info, message: "hev-socks5-tunnel started for Hysteria2")
            completionHandler(nil)
            Socks5Tunnel.run(withConfig: configPath)
        }
    }
}
