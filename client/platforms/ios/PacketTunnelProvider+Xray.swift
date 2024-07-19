import Foundation
import NetworkExtension
import WireGuardKitGo

enum XrayErrors: Error {
    case noXrayConfig
    case cantSaveXrayConfig
    case cantParseListenAndPort
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
    func startXray(completionHandler: @escaping (Error?) -> Void) {

        // Xray configuration
        guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
              let providerConfiguration = protocolConfiguration.providerConfiguration,
              let xrayConfigData = providerConfiguration[Constants.xrayConfigKey] as? Data else {
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

        let dns = ["8.8.4.4","1.1.1.1"]
        settings.dnsSettings = NEDNSSettings(servers: dns)

        do {
            let port = 10808
            let address = "::1"

            let jsonDict = try JSONSerialization.jsonObject(with: xrayConfigData,
                                                            options: []) as? [String: Any]

            guard var jsonDict else {
                xrayLog(.error, message: "Can't parse address and port for hevSocks")
                completionHandler(XrayErrors.cantParseListenAndPort)
                return
            }

            if var inboundsArray = jsonDict["inbounds"] as? [[String: Any]], !inboundsArray.isEmpty {
                inboundsArray[0]["port"] = port
                inboundsArray[0]["listen"] = address
                jsonDict["inbounds"] = inboundsArray
            }

            let updatedData = try JSONSerialization.data(withJSONObject: jsonDict, options: [])

            setTunnelNetworkSettings(settings) { [weak self] error in
                if let error {
                    completionHandler(error)
                    return
                }

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

    private func setupAndStartXray(configData: Data,
                                   completionHandler: @escaping (Error?) -> Void) {
        let path = Constants.cachesDirectory.appendingPathComponent("config.json", isDirectory: false).path
        guard FileManager.default.createFile(atPath: path, contents: configData) else {
            xrayLog(.error, message: "Can't save xray configuration")
            completionHandler(XrayErrors.cantSaveXrayConfig)
            return
        }

        LibXrayRunXray(nil,
                       path,
                       Int64.max)

        completionHandler(nil)
        xrayLog(.info, message: "Xray started")
    }

    private func setupAndRunTun2socks(configData: Data,
                                      address: String,
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
