import Foundation
import NetworkExtension
import OpenVPNAdapter
import CryptoKit

struct OpenVPNConfig: Decodable {
    let config: String
    let splitTunnelType: Int
    let splitTunnelSites: [String]

    var str: String {
        "splitTunnelType: \(splitTunnelType) splitTunnelSites: \(splitTunnelSites) config: \(config)"
    }
}

extension PacketTunnelProvider {
    func startOpenVPN(completionHandler: @escaping (Error?) -> Void) {
        guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
              let providerConfiguration = protocolConfiguration.providerConfiguration,
              let openVPNConfigData = providerConfiguration[Constants.ovpnConfigKey] as? Data else {
            ovpnLog(.error, message: "Can't start")
            return
        }

        do {
            let openVPNConfig = try JSONDecoder().decode(OpenVPNConfig.self, from: openVPNConfigData)
            ovpnLog(.info, title: "config: ", message: openVPNConfig.str)
            let ovpnConfiguration = Data(openVPNConfig.config.utf8)
            setupAndlaunchOpenVPN(withConfig: ovpnConfiguration, completionHandler: completionHandler)
        } catch {
            ovpnLog(.error, message: "Can't parse OpenVPN config: \(error.localizedDescription)")
            return
        }
    }

    private func logOpenVPNError(_ error: NSError) {
        let fatalFlag = (error.userInfo[OpenVPNAdapterErrorFatalKey] as? Bool) ?? false
        var lines: [String] = []
        lines.append("domain=\(error.domain) code=\(error.code) fatal=\(fatalFlag)")

        if let adapterMessage = error.userInfo[OpenVPNAdapterErrorMessageKey] as? String, !adapterMessage.isEmpty {
            lines.append("message=\(adapterMessage)")
        }

        let userInfoKeys = error.userInfo.keys.map { String(describing: $0) }.sorted()
        if !userInfoKeys.isEmpty {
            lines.append("userInfoKeys=[\(userInfoKeys.joined(separator: ","))]")
        }

        if let underlying = error.userInfo[NSUnderlyingErrorKey] as? NSError {
            lines.append("underlying=\(underlying.domain)#\(underlying.code) fatal=\((underlying.userInfo[OpenVPNAdapterErrorFatalKey] as? Bool) ?? false)")
            if let underlyingMessage = underlying.userInfo[OpenVPNAdapterErrorMessageKey] as? String, !underlyingMessage.isEmpty {
                lines.append("underlyingMessage=\(underlyingMessage)")
            } else if !underlying.localizedDescription.isEmpty {
                lines.append("underlyingLocalized=\(underlying.localizedDescription)")
            }
        } else if let underlying = error.userInfo[NSUnderlyingErrorKey] {
            lines.append("underlyingRaw=\(underlying)")
        }

        let formatted = lines.joined(separator: "\n  ")
        ovpnLog(.error, title: "Error", message: formatted)
    }

    private func setupAndlaunchOpenVPN(withConfig ovpnConfiguration: Data,
                                       withShadowSocks viaSS: Bool = false,
                                       completionHandler: @escaping (Error?) -> Void) {
        ovpnLog(.info, message: "Setup and launch")

        var configString = String(decoding: ovpnConfiguration, as: UTF8.self)

        let digest = SHA256.hash(data: ovpnConfiguration)
        let digestString = digest.map { String(format: "%02x", $0) }.joined()
        ovpnLog(.info, title: "ConfigDigest", message: digestString)

        let hasTlsAuthOpen = configString.contains("<tls-auth>")
        let hasTlsAuthClose = configString.contains("</tls-auth>")
        ovpnLog(.info, title: "ConfigFlags", message: "tls-auth open=\(hasTlsAuthOpen) close=\(hasTlsAuthClose)")

        let lines = configString.split(separator: "\n")
        let head = lines.prefix(10).joined(separator: "\n")
        let tail = lines.suffix(10).joined(separator: "\n")
        ovpnLog(.debug, title: "ConfigHead", message: head)
        ovpnLog(.debug, title: "ConfigTail", message: tail)

        if let start = configString.range(of: "<tls-auth>"),
           let end = configString.range(of: "</tls-auth>", range: start.upperBound..<configString.endIndex) {
            let keyBody = String(configString[start.upperBound..<end.lowerBound])
            ovpnLog(.debug, title: "TLSAuthInline", message: keyBody)
            let sanitizedLines = keyBody
                .split(whereSeparator: { $0.isNewline })
                .map { $0.trimmingCharacters(in: .whitespacesAndNewlines) }
                .filter { !$0.isEmpty }
                .filter { !$0.hasPrefix("#") }

            let sanitizedKey = sanitizedLines.joined(separator: "\n")
            ovpnLog(.debug, title: "TLSAuthSanitized", message: sanitizedKey)
            let sanitizedBlock = "<tls-auth>\n\(sanitizedKey)\n</tls-auth>"
            configString.replaceSubrange(start.lowerBound..<end.upperBound, with: sanitizedBlock)
        }

        let normalizedConfig = configString.replacingOccurrences(of: "\r\n", with: "\n")
        let sanitizedData = Data(normalizedConfig.utf8)

        let configuration = OpenVPNConfiguration()
        configuration.fileContent = sanitizedData
        if configString.contains("cloak") {
            configuration.setPTCloak()
        }

        let evaluation: OpenVPNConfigurationEvaluation?
        do {
            ovpnAdapter = OpenVPNAdapter()
            ovpnAdapter?.delegate = self
            evaluation = try ovpnAdapter?.apply(configuration: configuration)

        } catch {
            let nsError = error as NSError
            ovpnLog(.error, title: "ApplyConfig", message: "domain=\(nsError.domain) code=\(nsError.code) info=\(nsError.userInfo)")
            completionHandler(error)
            return
        }

        if evaluation?.autologin == false {
            ovpnLog(.info, message: "Implement login with user credentials")
        }

        vpnReachability.startTracking { [weak self] status in
            guard status == .reachableViaWiFi else { return }
            self?.ovpnAdapter?.reconnect(afterTimeInterval: 5)
        }

        startHandler = completionHandler
        ovpnAdapter?.connect(using: packetFlow)
    }

    func handleOpenVPNStatusMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let completionHandler = completionHandler else { return }
        let bytesin = ovpnAdapter?.transportStatistics.bytesIn
        let bytesout = ovpnAdapter?.transportStatistics.bytesOut

        guard let bytesin, let bytesout else {
            completionHandler(nil)
            return
        }

        let response: [String: Any] = [
            "rx_bytes": bytesin,
            "tx_bytes": bytesout
        ]

        completionHandler(try? JSONSerialization.data(withJSONObject: response, options: []))
    }

    func stopOpenVPN(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        ovpnLog(.info, message: "Stopping tunnel: reason: \(reason.description)")

        stopHandler = completionHandler
        if vpnReachability.isTracking {
            vpnReachability.stopTracking()
        }
        ovpnAdapter?.disconnect()
    }
}

extension PacketTunnelProvider: OpenVPNAdapterDelegate {
    // OpenVPNAdapter calls this delegate method to configure a VPN tunnel.
    // `completionHandler` callback requires an object conforming to `OpenVPNAdapterPacketFlow`
    // protocol if the tunnel is configured without errors. Otherwise send nil.
    // `OpenVPNAdapterPacketFlow` method signatures are similar to `NEPacketTunnelFlow` so
    // you can just extend that class to adopt `OpenVPNAdapterPacketFlow` protocol and
    // send `self.packetFlow` to `completionHandler` callback.
    func openVPNAdapter(
        _ openVPNAdapter: OpenVPNAdapter,
        configureTunnelWithNetworkSettings networkSettings: NEPacketTunnelNetworkSettings?,
        completionHandler: @escaping (Error?) -> Void
    ) {
        // In order to direct all DNS queries first to the VPN DNS servers before the primary DNS servers
        // send empty string to NEDNSSettings.matchDomains
        networkSettings?.dnsSettings?.matchDomains = [""]

        if splitTunnelType == 1 {
            var ipv4IncludedRoutes = [NEIPv4Route]()

            guard let splitTunnelSites else {
                completionHandler(NSError(domain: "Split tunnel sited not setted up", code: 0))
                return
            }

            for allowedIPString in splitTunnelSites {
                if let allowedIP = IPAddressRange(from: allowedIPString) {
                    ipv4IncludedRoutes.append(NEIPv4Route(
                        destinationAddress: "\(allowedIP.address)",
                        subnetMask: "\(allowedIP.subnetMask())"))
                }
            }

            networkSettings?.ipv4Settings?.includedRoutes = ipv4IncludedRoutes
        } else {
            if splitTunnelType == 2 {
                var ipv4ExcludedRoutes = [NEIPv4Route]()
                var ipv4IncludedRoutes = [NEIPv4Route]()
                var ipv6IncludedRoutes = [NEIPv6Route]()

                guard let splitTunnelSites else {
                    completionHandler(NSError(domain: "Split tunnel sited not setted up", code: 0))
                    return
                }

                for excludeIPString in splitTunnelSites {
                    if let excludeIP = IPAddressRange(from: excludeIPString) {
                        ipv4ExcludedRoutes.append(NEIPv4Route(
                            destinationAddress: "\(excludeIP.address)",
                            subnetMask: "\(excludeIP.subnetMask())"))
                    }
                }

                if let allIPv4 = IPAddressRange(from: "0.0.0.0/0") {
                    ipv4IncludedRoutes.append(NEIPv4Route(
                        destinationAddress: "\(allIPv4.address)",
                        subnetMask: "\(allIPv4.subnetMask())"))
                }
                if let allIPv6 = IPAddressRange(from: "::/0") {
                    ipv6IncludedRoutes.append(NEIPv6Route(
                        destinationAddress: "\(allIPv6.address)",
                        networkPrefixLength: NSNumber(value: allIPv6.networkPrefixLength)))
                }
                networkSettings?.ipv4Settings?.includedRoutes = ipv4IncludedRoutes
                networkSettings?.ipv6Settings?.includedRoutes = ipv6IncludedRoutes
                networkSettings?.ipv4Settings?.excludedRoutes = ipv4ExcludedRoutes
            }
        }

        // Set the network settings for the current tunneling session.
        setTunnelNetworkSettings(networkSettings, completionHandler: completionHandler)
    }

    // Process events returned by the OpenVPN library
    func openVPNAdapter(
        _ openVPNAdapter: OpenVPNAdapter,
        handleEvent event: OpenVPNAdapterEvent,
        message: String?) {
            switch event {
            case .connected:
                if reasserting {
                    reasserting = false
                }

                guard let startHandler = startHandler else { return }

                startHandler(nil)
                self.startHandler = nil
            case .disconnected:
                guard let stopHandler = stopHandler else { return }

                if vpnReachability.isTracking {
                    vpnReachability.stopTracking()
                }

                stopHandler()
                self.stopHandler = nil
            case .reconnecting:
                reasserting = true
            default:
                break
            }
        }

    // Handle errors thrown by the OpenVPN library
    func openVPNAdapter(_ openVPNAdapter: OpenVPNAdapter, handleError error: Error) {
        let nsError = error as NSError
        logOpenVPNError(nsError)

        // Handle only fatal errors
        guard let fatal = nsError.userInfo[OpenVPNAdapterErrorFatalKey] as? Bool,
              fatal == true else { return }

        if vpnReachability.isTracking {
            vpnReachability.stopTracking()
        }

        if let startHandler {
            startHandler(error)
            self.startHandler = nil
        } else {
            cancelTunnelWithError(error)
        }
    }

    // Use this method to process any log message returned by OpenVPN library.
    func openVPNAdapter(_ openVPNAdapter: OpenVPNAdapter, handleLogMessage logMessage: String) {
        // Handle log messages
        ovpnLog(.info, message: logMessage)
    }
}

extension NEPacketTunnelFlow: OpenVPNAdapterPacketFlow {}
