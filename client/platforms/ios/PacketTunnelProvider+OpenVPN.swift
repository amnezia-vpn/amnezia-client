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
        // Reset session-derived state so reconnects never reuse stale gateway/address data.
        openVpnGatewayAddress = nil
        openVpnLocalAddress = nil
        openVpnLocalMask = nil
        lastOpenVPNSettings = nil

        guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
              let providerConfiguration = protocolConfiguration.providerConfiguration,
              let openVPNConfigData = providerConfiguration[Constants.ovpnConfigKey] as? Data else {
            ovpnLog(.error, message: "Can't start")
            return
        }

        do {
            let openVPNConfig = try JSONDecoder().decode(OpenVPNConfig.self, from: openVPNConfigData)
            ovpnLog(.info, title: "config: ", message: openVPNConfig.str)
            let wrapperPreview = String(decoding: openVPNConfigData.prefix(512), as: UTF8.self)
            let ovpnPreview = String(openVPNConfig.config.prefix(512))
            ovpnLog(.info, title: "config wrapper", message: "bytes=\(openVPNConfigData.count) preview=\(wrapperPreview)")
            ovpnLog(.info, title: "config raw", message: "chars=\(openVPNConfig.config.count) preview=\(ovpnPreview)")
            let ovpnConfiguration = Data(openVPNConfig.config.utf8)
            splitTunnelType = openVPNConfig.splitTunnelType
            splitTunnelSites = openVPNConfig.splitTunnelSites
            openVpnDnsServers = Self.extractDnsServers(from: openVPNConfig.config)
            openVpnRemoteAddress = Self.extractRemoteHost(from: openVPNConfig.config)
            openVpnRedirectGatewayDef1 = Self.hasRedirectGatewayDef1(in: openVPNConfig.config)
            if let openVpnRemoteAddress {
                ovpnLog(.info, title: "Remote", message: "host=\(openVpnRemoteAddress)")
            }
            if !openVpnDnsServers.isEmpty {
                ovpnLog(.info, title: "DNS", message: "servers=\(openVpnDnsServers)")
            }
            if openVpnRedirectGatewayDef1 {
                ovpnLog(.info, title: "IPv4Routes", message: "redirect-gateway def1 detected")
            }
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
                                       completionHandler: @escaping (Error?) -> Void) {
        ovpnLog(.info, message: "Setup and launch")

        var configString = String(decoding: ovpnConfiguration, as: UTF8.self)

        let digest = SHA256.hash(data: ovpnConfiguration)
        let digestString = digest.map { String(format: "%02x", $0) }.joined()
        ovpnLog(.info, title: "ConfigDigest", message: digestString)

        let hasCertTag = configString.contains("<cert>") && configString.contains("</cert>")
        let hasKeyTag = configString.contains("<key>") && configString.contains("</key>")
        let hasAuthUserPass = configString.contains("auth-user-pass")
        ovpnLog(.info, title: "ConfigCreds", message: "inlineCert=\(hasCertTag) inlineKey=\(hasKeyTag) authUserPass=\(hasAuthUserPass)")

        let hasTlsAuthOpen = configString.contains("<tls-auth>")
        let hasTlsAuthClose = configString.contains("</tls-auth>")
        ovpnLog(.info, title: "ConfigFlags", message: "tls-auth open=\(hasTlsAuthOpen) close=\(hasTlsAuthClose)")

        let lines = configString.split(separator: "\n")
        let head = lines.prefix(10).joined(separator: "\n")
        let tail = lines.suffix(10).joined(separator: "\n")
        ovpnLog(.debug, title: "ConfigHead", message: head)
        ovpnLog(.debug, title: "ConfigTail", message: tail)

        if hasTlsAuthOpen && hasTlsAuthClose {
            ovpnLog(.info, title: "TLSAuthSanitized", message: "preserve original tls-auth block")
        }

        var normalizedConfig = configString.replacingOccurrences(of: "\r\n", with: "\n")
        normalizedConfig = Self.normalizeInlineBlock(
            in: normalizedConfig,
            tag: "ca",
            beginMarkers: ["-----BEGIN CERTIFICATE-----"],
            endMarkers: ["-----END CERTIFICATE-----"]
        )
        normalizedConfig = Self.normalizeInlineBlock(
            in: normalizedConfig,
            tag: "cert",
            beginMarkers: ["-----BEGIN CERTIFICATE-----"],
            endMarkers: ["-----END CERTIFICATE-----"]
        )
        normalizedConfig = Self.normalizeInlineBlock(
            in: normalizedConfig,
            tag: "key",
            beginMarkers: [
                "-----BEGIN PRIVATE KEY-----",
                "-----BEGIN RSA PRIVATE KEY-----",
                "-----BEGIN EC PRIVATE KEY-----",
                "-----BEGIN ENCRYPTED PRIVATE KEY-----"
            ],
            endMarkers: [
                "-----END PRIVATE KEY-----",
                "-----END RSA PRIVATE KEY-----",
                "-----END EC PRIVATE KEY-----",
                "-----END ENCRYPTED PRIVATE KEY-----"
            ]
        )
        normalizedConfig = Self.normalizeInlineBlock(
            in: normalizedConfig,
            tag: "tls-auth",
            beginMarkers: ["-----BEGIN OpenVPN Static key V1-----"],
            endMarkers: ["-----END OpenVPN Static key V1-----"]
        )
        normalizedConfig = Self.stripUnsupportedOptions(forOpenVPNAdapter: normalizedConfig)
        if !normalizedConfig.hasSuffix("\n") {
            normalizedConfig.append("\n")
        }
        let normalizedLines = normalizedConfig.split(whereSeparator: \.isNewline)
        let normalizedTail = normalizedLines.suffix(10).joined(separator: "\n")
        ovpnLog(.debug, title: "ConfigTailSanitized", message: normalizedTail)
        let redirectLines = normalizedLines
            .map(String.init)
            .filter { $0.lowercased().contains("redirect-gateway") }
        if !redirectLines.isEmpty {
            ovpnLog(.info, title: "ConfigRedirect", message: redirectLines.joined(separator: " | "))
        }
        let controlScalars = normalizedConfig.unicodeScalars.filter {
            ($0.value < 0x20 && $0 != "\n" && $0 != "\r" && $0 != "\t")
        }
        if !controlScalars.isEmpty {
            ovpnLog(.error, title: "ConfigChars", message: "nonPrintableControlCount=\(controlScalars.count)")
        }
#if os(macOS)
        let dumpBaseURL = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first
            ?? FileManager.default.temporaryDirectory
        let dumpURL = dumpBaseURL.appendingPathComponent("amnezia_ovpn_adapter_config.conf")
        do {
            try normalizedConfig.write(to: dumpURL, atomically: true, encoding: .utf8)
            ovpnLog(.info, title: "ConfigDump", message: "path=\(dumpURL.path) bytes=\(normalizedConfig.utf8.count)")
        } catch {
            ovpnLog(.error, title: "ConfigDump", message: "write failed: \(error.localizedDescription)")
        }
#endif
        let sanitizedData = Data(normalizedConfig.utf8)

        let configuration = OpenVPNConfiguration()
        configuration.fileContent = sanitizedData
        // Be explicit: enum default is 0 (enabled), we need stubs-only behavior.
        configuration.compressionMode = .disabled
        // A-012: emulate OpenVPN2 CLI capability advertisement as closely as possible.
        configuration.peerInfo = [
            "IV_VER": "2.6.10",
            "IV_PLAT": "mac",
            "IV_TCPNL": "1",
            "IV_MTU": "1600",
            "IV_NCP": "2",
            "IV_CIPHERS": "AES-256-GCM:AES-128-GCM:CHACHA20-POLY1305",
            "IV_PROTO": "990",
            "IV_LZO_STUB": "1",
            "IV_COMP_STUB": "1",
            "IV_COMP_STUBv2": "1"
        ]
        if let peerInfo = configuration.peerInfo {
            let peerInfoSummary = peerInfo.keys.sorted().map { "\($0)=\(peerInfo[$0] ?? "")" }.joined(separator: " ")
            ovpnLog(.info, title: "PeerInfoOverride", message: peerInfoSummary)
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
        if let evaluation {
            ovpnLog(.info, title: "ConfigEval", message: "autologin=\(evaluation.autologin) externalPki=\(evaluation.externalPki)")
        }

#if !os(macOS)
        vpnReachability.startTracking { [weak self] status in
            self?.handleOpenVPNReachabilityChange(status)
        }
#endif

        startHandler = completionHandler
        ovpnAdapter?.connect(using: openVPNPacketFlow())
    }

    func handleOpenVPNStatusMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let completionHandler = completionHandler else { return }
        let bytesin = ovpnAdapter?.transportStatistics.bytesIn
        let bytesout = ovpnAdapter?.transportStatistics.bytesOut

        guard let bytesin, let bytesout else {
            completionHandler(nil)
            return
        }

        ovpnLog(.info, title: "Transport", message: "bytesIn=\(bytesin) bytesOut=\(bytesout)")

        let response: [String: Any] = [
            "rx_bytes": bytesin,
            "tx_bytes": bytesout
        ]

        completionHandler(try? JSONSerialization.data(withJSONObject: response, options: []))
    }

    func stopOpenVPN(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        ovpnLog(.info, message: "Stopping tunnel: reason: \(reason.amneziaDescription)")

        stopHandler = completionHandler
        openVpnGatewayAddress = nil
        openVpnLocalAddress = nil
        openVpnLocalMask = nil
        lastOpenVPNSettings = nil
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
        guard var effectiveSettings = networkSettings else {
            ovpnLog(.info, title: "SetTunnelNetworkSettings", message: "nil settings; skipping update")
            completionHandler(nil)
            return
        }
        let splitType = splitTunnelType ?? 0

        if let ipv4Settings = effectiveSettings.ipv4Settings {
            openVpnLocalAddress = ipv4Settings.addresses.first
            openVpnLocalMask = ipv4Settings.subnetMasks.first
        }

        let serverIP = openVPNAdapter.connectionInformation?.serverIP
        let configRemote = openVpnRemoteAddress
        let serverEndpoint: String? = {
            if let ip = serverIP, Self.isIPv4Address(ip) { return ip }
            if let ip = configRemote, Self.isIPv4Address(ip) { return ip }
            return effectiveSettings.tunnelRemoteAddress
        }()

        if let serverEndpoint,
           Self.isIPv4Address(serverEndpoint),
           effectiveSettings.tunnelRemoteAddress != serverEndpoint {
            let updatedSettings = NEPacketTunnelNetworkSettings(tunnelRemoteAddress: serverEndpoint)
            updatedSettings.ipv4Settings = effectiveSettings.ipv4Settings
            updatedSettings.ipv6Settings = effectiveSettings.ipv6Settings
            updatedSettings.dnsSettings = effectiveSettings.dnsSettings
            updatedSettings.proxySettings = effectiveSettings.proxySettings
            updatedSettings.mtu = effectiveSettings.mtu
            effectiveSettings = updatedSettings
            ovpnLog(.info, title: "Remote", message: "tunnelRemoteAddress set to server=\(serverEndpoint)")
        } else if let serverEndpoint, !Self.isIPv4Address(serverEndpoint) {
            ovpnLog(.info, title: "Remote", message: "skip tunnelRemoteAddress override; non-ip serverEndpoint=\(serverEndpoint)")
        }

        // In order to direct all DNS queries first to the VPN DNS servers before the primary DNS servers
        // send empty string to NEDNSSettings.matchDomains
        if let dnsSettings = effectiveSettings.dnsSettings {
            if dnsSettings.servers.isEmpty, !openVpnDnsServers.isEmpty {
                let newSettings = NEDNSSettings(servers: openVpnDnsServers)
                newSettings.matchDomains = dnsSettings.matchDomains
                effectiveSettings.dnsSettings = newSettings
            }
        } else if !openVpnDnsServers.isEmpty {
            let newSettings = NEDNSSettings(servers: openVpnDnsServers)
            effectiveSettings.dnsSettings = newSettings
        }

        effectiveSettings.dnsSettings?.matchDomains = [""]
        if let dnsSettings = effectiveSettings.dnsSettings {
            let servers = dnsSettings.servers.joined(separator: ",")
            let domains = dnsSettings.matchDomains?.joined(separator: ",") ?? ""
            ovpnLog(.info, title: "DNS", message: "servers=[\(servers)] matchDomains=[\(domains)]")
        } else {
            ovpnLog(.error, title: "DNS", message: "dnsSettings is nil")
        }

        let tunnelRemote = effectiveSettings.tunnelRemoteAddress
        if !tunnelRemote.isEmpty {
            ovpnLog(.info, title: "Remote", message: "tunnelRemoteAddress=\(tunnelRemote)")
        } else if let remoteAddress = openVpnRemoteAddress {
            ovpnLog(.info, title: "Remote", message: "tunnelRemoteAddress is empty, configRemote=\(remoteAddress)")
        }

        if let ipv4Settings = effectiveSettings.ipv4Settings {
            let included = (ipv4Settings.includedRoutes ?? []).map { "\($0.destinationAddress)/\($0.destinationSubnetMask)" }
            let excluded = (ipv4Settings.excludedRoutes ?? []).map { "\($0.destinationAddress)/\($0.destinationSubnetMask)" }
            let addresses = ipv4Settings.addresses.joined(separator: ",")
            let masks = ipv4Settings.subnetMasks.joined(separator: ",")
            let router: String
#if os(macOS)
            if #available(macOS 13.0, *) {
                router = ipv4Settings.router ?? ""
            } else {
                router = ""
            }
#else
            router = ""
#endif
            ovpnLog(.info, title: "IPv4RoutesPre", message: "addresses=[\(addresses)] masks=[\(masks)] router=\(router) included=\(included) excluded=\(excluded)")
        } else {
            ovpnLog(.error, title: "IPv4RoutesPre", message: "ipv4Settings is nil")
        }

        if let ipv6Settings = effectiveSettings.ipv6Settings {
            let included = (ipv6Settings.includedRoutes ?? []).map { "\($0.destinationAddress)/\($0.destinationNetworkPrefixLength)" }
            let excluded = (ipv6Settings.excludedRoutes ?? []).map { "\($0.destinationAddress)/\($0.destinationNetworkPrefixLength)" }
            let addresses = ipv6Settings.addresses.joined(separator: ",")
            let prefixes = ipv6Settings.networkPrefixLengths.map { "\($0)" }.joined(separator: ",")
            ovpnLog(.info, title: "IPv6RoutesPre", message: "addresses=[\(addresses)] prefixes=[\(prefixes)] included=\(included) excluded=\(excluded)")
        }

        if splitType == 1 {
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

            effectiveSettings.ipv4Settings?.includedRoutes = ipv4IncludedRoutes
        } else if splitType == 2 {
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
                effectiveSettings.ipv4Settings?.includedRoutes = ipv4IncludedRoutes
                effectiveSettings.ipv6Settings?.includedRoutes = ipv6IncludedRoutes
                effectiveSettings.ipv4Settings?.excludedRoutes = ipv4ExcludedRoutes
        } else {
            // Full tunnel: rely on adapter-provided routes.
        }

        if let serverEndpoint,
           Self.isIPv4Address(serverEndpoint),
           let ipv4Settings = effectiveSettings.ipv4Settings {
            let hostMask = "255.255.255.255"
            var excluded = ipv4Settings.excludedRoutes ?? []
            let alreadyExcluded = excluded.contains {
                $0.destinationAddress == serverEndpoint && $0.destinationSubnetMask == hostMask
            }
            if !alreadyExcluded {
                excluded.append(NEIPv4Route(destinationAddress: serverEndpoint, subnetMask: hostMask))
                ipv4Settings.excludedRoutes = excluded
                ovpnLog(.info, title: "IPv4Routes", message: "excluded remoteAddress=\(serverEndpoint)")
            }
        } else if let serverEndpoint {
            ovpnLog(.info, title: "IPv4Routes", message: "skip explicit remote exclude; non-ip server=\(serverEndpoint)")
        }

        let localAddr = openVpnLocalAddress
        var net30Gateway: String?
        if let localAddr, let mask = openVpnLocalMask {
            net30Gateway = Self.net30Peer(for: localAddr, mask: mask)
        }
        var gateway = net30Gateway
        if let adapterGateway = openVPNAdapter.connectionInformation?.gatewayIPv4, !adapterGateway.isEmpty {
            if let localAddr, adapterGateway == localAddr {
                ovpnLog(.info, title: "IPv4Gateway", message: "ignore adapter gateway equal to local address=\(adapterGateway)")
            } else if let net30Gateway, net30Gateway != adapterGateway {
                ovpnLog(.info, title: "IPv4Gateway", message: "ignore mismatched adapter gateway=\(adapterGateway), using net30 peer=\(net30Gateway)")
            } else {
                gateway = adapterGateway
            }
        }

        openVpnGatewayAddress = gateway
        if let gateway, !gateway.isEmpty {
            ovpnLog(.info, title: "IPv4Gateway", message: "gateway=\(gateway)")
        }
#if os(macOS)
        if splitType == 0, let gateway, !gateway.isEmpty, effectiveSettings.tunnelRemoteAddress != gateway {
            let updatedSettings = NEPacketTunnelNetworkSettings(tunnelRemoteAddress: gateway)
            updatedSettings.ipv4Settings = effectiveSettings.ipv4Settings
            updatedSettings.ipv6Settings = effectiveSettings.ipv6Settings
            updatedSettings.dnsSettings = effectiveSettings.dnsSettings
            updatedSettings.proxySettings = effectiveSettings.proxySettings
            updatedSettings.mtu = effectiveSettings.mtu
            effectiveSettings = updatedSettings
            ovpnLog(.info, title: "Remote", message: "tunnelRemoteAddress set to gateway=\(gateway) on macOS full-tunnel")
        }
#endif
#if os(macOS)
        if var ipv4Settings = effectiveSettings.ipv4Settings {
            if splitType == 0 {
                let hasNet30Mask = ipv4Settings.subnetMasks.contains("255.255.255.252")
                if hasNet30Mask {
                    let normalizedMasks = Array(repeating: "255.255.255.255",
                                                count: ipv4Settings.subnetMasks.count)
                    let normalized = NEIPv4Settings(addresses: ipv4Settings.addresses,
                                                    subnetMasks: normalizedMasks)
                    normalized.includedRoutes = ipv4Settings.includedRoutes
                    normalized.excludedRoutes = ipv4Settings.excludedRoutes
                    if #available(macOS 13.0, *) {
                        normalized.router = ipv4Settings.router
                    }
                    ipv4Settings = normalized
                    ovpnLog(.info, title: "IPv4Routes", message: "normalized net30 /30 masks to /32 on macOS full-tunnel")
                }

                if let gateway, !gateway.isEmpty {
                    if #available(macOS 13.0, *) {
                        ipv4Settings.router = gateway
                        ovpnLog(.info, title: "IPv4Routes", message: "set ipv4 router=\(gateway) on macOS full-tunnel")
                    }
                }

                var included = ipv4Settings.includedRoutes ?? []
                let hasDefault = included.contains {
                    $0.destinationAddress == "0.0.0.0" && $0.destinationSubnetMask == "0.0.0.0"
                }
                if hasDefault {
                    included.removeAll {
                        $0.destinationAddress == "0.0.0.0" && $0.destinationSubnetMask == "0.0.0.0"
                    }
                }
                let hasDef1Low = included.contains {
                    $0.destinationAddress == "0.0.0.0" && $0.destinationSubnetMask == "128.0.0.0"
                }
                let hasDef1High = included.contains {
                    $0.destinationAddress == "128.0.0.0" && $0.destinationSubnetMask == "128.0.0.0"
                }
                if (hasDefault || openVpnRedirectGatewayDef1) && !(hasDef1Low && hasDef1High) {
                    if !hasDef1Low {
                        let route = NEIPv4Route(destinationAddress: "0.0.0.0", subnetMask: "128.0.0.0")
                        if let gateway, !gateway.isEmpty {
                            route.gatewayAddress = gateway
                        }
                        included.append(route)
                    }
                    if !hasDef1High {
                        let route = NEIPv4Route(destinationAddress: "128.0.0.0", subnetMask: "128.0.0.0")
                        if let gateway, !gateway.isEmpty {
                            route.gatewayAddress = gateway
                        }
                        included.append(route)
                    }
                    ovpnLog(.info, title: "IPv4Routes", message: "ensured def1 routes (/1 + /1) on macOS full-tunnel")
                }
                if let gateway, !gateway.isEmpty {
                    included = included.map { route in
                        let isDef1 =
                            (route.destinationAddress == "0.0.0.0" && route.destinationSubnetMask == "128.0.0.0") ||
                            (route.destinationAddress == "128.0.0.0" && route.destinationSubnetMask == "128.0.0.0")
                        guard isDef1 else { return route }
                        if route.gatewayAddress == gateway {
                            return route
                        }
                        let updatedRoute = NEIPv4Route(destinationAddress: route.destinationAddress,
                                                       subnetMask: route.destinationSubnetMask)
                        updatedRoute.gatewayAddress = gateway
                        return updatedRoute
                    }
                    ovpnLog(.info, title: "IPv4Routes", message: "set gateway=\(gateway) on macOS def1 routes")
                }
                ipv4Settings.includedRoutes = included
                effectiveSettings.ipv4Settings = ipv4Settings
            }
        }
#endif
        if let ipv4Settings = effectiveSettings.ipv4Settings {
            let included = (ipv4Settings.includedRoutes ?? []).map {
                let gw = $0.gatewayAddress ?? ""
                return "\($0.destinationAddress)/\($0.destinationSubnetMask) gw=\(gw)"
            }
            let excluded = (ipv4Settings.excludedRoutes ?? []).map { "\($0.destinationAddress)/\($0.destinationSubnetMask)" }
            let addresses = ipv4Settings.addresses.joined(separator: ",")
            let masks = ipv4Settings.subnetMasks.joined(separator: ",")
            let router: String
#if os(macOS)
            if #available(macOS 13.0, *) {
                router = ipv4Settings.router ?? ""
            } else {
                router = ""
            }
#else
            router = ""
#endif
            ovpnLog(.info, title: "IPv4Routes", message: "addresses=[\(addresses)] masks=[\(masks)] router=\(router) included=\(included) excluded=\(excluded)")
        } else {
            ovpnLog(.error, title: "IPv4Routes", message: "ipv4Settings is nil")
        }

        if let ipv6Settings = effectiveSettings.ipv6Settings {
            let included = (ipv6Settings.includedRoutes ?? []).map { "\($0.destinationAddress)/\($0.destinationNetworkPrefixLength)" }
            let excluded = (ipv6Settings.excludedRoutes ?? []).map { "\($0.destinationAddress)/\($0.destinationNetworkPrefixLength)" }
            let addresses = ipv6Settings.addresses.joined(separator: ",")
            let prefixes = ipv6Settings.networkPrefixLengths.map { "\($0)" }.joined(separator: ",")
            ovpnLog(.info, title: "IPv6Routes", message: "addresses=[\(addresses)] prefixes=[\(prefixes)] included=\(included) excluded=\(excluded)")
        }
#if os(macOS)
        if effectiveSettings.ipv6Settings != nil {
            effectiveSettings.ipv6Settings = nil
            ovpnLog(.info, title: "IPv6", message: "cleared ipv6Settings on macOS")
        }
#endif

        lastOpenVPNSettings = effectiveSettings

        // Set the network settings for the current tunneling session.
        setTunnelNetworkSettings(effectiveSettings) { error in
            if let error {
                ovpnLog(.error, title: "SetTunnelNetworkSettings", message: error.localizedDescription)
            } else {
                ovpnLog(.info, title: "SetTunnelNetworkSettings", message: "ok")
            }
            completionHandler(error)
        }
    }

    private static func extractDnsServers(from config: String) -> [String] {
        let lines = config.split(whereSeparator: \.isNewline)
        var servers: [String] = []
        for line in lines {
            let trimmed = line.trimmingCharacters(in: .whitespacesAndNewlines)
            if trimmed.hasPrefix("dhcp-option DNS ") {
                let parts = trimmed.split(separator: " ")
                if let last = parts.last {
                    servers.append(String(last))
                }
            }
        }
        return servers
    }

    private static func extractRemoteHost(from config: String) -> String? {
        let lines = config.split(whereSeparator: \.isNewline)
        for line in lines {
            let trimmed = line.trimmingCharacters(in: .whitespacesAndNewlines)
            if trimmed.hasPrefix("remote ") {
                let parts = trimmed.split(separator: " ")
                if parts.count >= 2 {
                    return String(parts[1])
                }
            }
        }
        return nil
    }

    private static func hasRedirectGatewayDef1(in config: String) -> Bool {
        let lines = config.split(whereSeparator: \.isNewline)
        for line in lines {
            let trimmed = line.trimmingCharacters(in: .whitespacesAndNewlines)
            if trimmed.hasPrefix("redirect-gateway") {
                return trimmed.split(whereSeparator: { $0 == " " || $0 == "\t" }).contains("def1")
            }
        }
        return false
    }

    private static func net30Peer(for address: String, mask: String) -> String? {
        guard mask == "255.255.255.252" else { return nil }
        let parts = address.split(separator: ".")
        guard parts.count == 4 else { return nil }
        var octets: [Int] = []
        for part in parts {
            guard let num = Int(part), num >= 0 && num <= 255 else { return nil }
            octets.append(num)
        }
        let ip = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3]
        let network = ip & ~3
        let host = ip - network
        let peerHost: Int
        switch host {
        case 1: peerHost = 2
        case 2: peerHost = 1
        default: return nil
        }
        let peerIP = network + peerHost
        return "\((peerIP >> 24) & 0xff).\((peerIP >> 16) & 0xff).\((peerIP >> 8) & 0xff).\(peerIP & 0xff)"
    }

    private func logOpenVPNConnectionInfo() {
        guard let info = ovpnAdapter?.connectionInformation else { return }
        let message = "vpnIPv4=\(info.vpnIPv4 ?? "") gatewayIPv4=\(info.gatewayIPv4 ?? "") serverIP=\(info.serverIP ?? "") tun=\(info.tunName ?? "")"
        ovpnLog(.info, title: "ConnInfo", message: message)
    }

    private static func normalizeInlineBlock(
        in config: String,
        tag: String,
        beginMarkers: [String],
        endMarkers: [String]
    ) -> String {
        guard !beginMarkers.isEmpty, !endMarkers.isEmpty else { return config }

        var normalizedConfig = config
        let openTag = "<\(tag)>"
        let closeTag = "</\(tag)>"
        var searchStart = normalizedConfig.startIndex

        while let openRange = normalizedConfig.range(of: openTag, range: searchStart..<normalizedConfig.endIndex),
              let closeRange = normalizedConfig.range(of: closeTag, range: openRange.upperBound..<normalizedConfig.endIndex) {
            let rawBody = String(normalizedConfig[openRange.upperBound..<closeRange.lowerBound])
            let lines = rawBody
                .split(whereSeparator: \.isNewline)
                .map { $0.trimmingCharacters(in: .whitespacesAndNewlines) }
                .filter { !$0.isEmpty }

            var beginIndex: Int?
            var endIndex: Int?
            for (idx, line) in lines.enumerated() {
                if beginIndex == nil,
                   beginMarkers.contains(where: { line.contains($0) }) {
                    beginIndex = idx
                }
                if beginIndex != nil,
                   endMarkers.contains(where: { line.contains($0) }) {
                    endIndex = idx
                }
            }

            if let beginIndex,
               let endIndex,
               endIndex >= beginIndex {
                let extracted = lines[beginIndex...endIndex].joined(separator: "\n")
                let replacement = "<\(tag)>\n\(extracted)\n</\(tag)>"
                normalizedConfig.replaceSubrange(openRange.lowerBound..<closeRange.upperBound, with: replacement)
                ovpnLog(.info, title: "ConfigInline", message: "tag=<\(tag)> linesIn=\(lines.count) linesOut=\(endIndex - beginIndex + 1)")
                searchStart = normalizedConfig.index(openRange.lowerBound, offsetBy: replacement.count)
            } else {
                ovpnLog(.error, title: "ConfigInline", message: "tag=<\(tag)> missing markers, keeping original body")
                searchStart = closeRange.upperBound
            }
        }

        return normalizedConfig
    }


    private static func stripUnsupportedOptions(forOpenVPNAdapter config: String) -> String {
        let unsupportedTokens: Set<String> = [
            "block-ipv6",
            "script-security",
            "up",
            "down",
            "resolv-retry",
            "persist-key",
            "persist-tun",
            "compat-mode",
            "disable-dco"
        ]
        let inlineBlockTags: Set<String> = [
            "ca",
            "cert",
            "key",
            "pkcs12",
            "tls-auth",
            "tls-crypt",
            "tls-crypt-v2",
            "secret",
            "crl-verify",
            "extra-certs"
        ]

        var removed: [String: Int] = [:]
        var normalized: [String: Int] = [:]
        var output: [String] = []
        var activeInlineTag: String?

        for rawLine in config.split(whereSeparator: \.isNewline) {
            let line = String(rawLine)
            let trimmed = line.trimmingCharacters(in: .whitespacesAndNewlines)
            if trimmed.isEmpty {
                output.append(line)
                continue
            }

            let trimmedLowercased = trimmed.lowercased()

            if let currentInlineTag = activeInlineTag {
                output.append(line)
                if trimmedLowercased == "</\(currentInlineTag)>" {
                    activeInlineTag = nil
                }
                continue
            }

            if trimmedLowercased.hasPrefix("<"),
               trimmedLowercased.hasSuffix(">"),
               !trimmedLowercased.hasPrefix("</") {
                let tagContent = String(trimmedLowercased.dropFirst().dropLast())
                let tagName = tagContent
                    .split(whereSeparator: { $0 == " " || $0 == "\t" })
                    .first
                    .map(String.init) ?? ""
                if inlineBlockTags.contains(tagName) {
                    activeInlineTag = tagName
                    output.append(line)
                    continue
                }
            }

            if trimmed.hasPrefix("#") || trimmed.hasPrefix(";") {
                output.append(line)
                continue
            }

            let parts = trimmed.split(whereSeparator: { $0 == " " || $0 == "\t" })
            let token = parts.first.map(String.init)?.lowercased() ?? ""
            if trimmedLowercased.hasPrefix("redirect-gateway") || token.hasPrefix("redirect-gateway") {
                let hasDef1 = parts.dropFirst().contains { String($0).lowercased().hasPrefix("def1") }
                if hasDef1 {
                    output.append("redirect-gateway def1")
                    normalized["redirect-gateway", default: 0] += 1
                } else {
                    removed["redirect-gateway", default: 0] += 1
                }
                continue
            }

            if let matchedUnsupported = unsupportedTokens.first(where: { token.hasPrefix($0) }) {
                removed[matchedUnsupported, default: 0] += 1
                continue
            }

            output.append(line)
        }

        if !removed.isEmpty {
            let summary = removed.keys.sorted().map { "\($0)=\(removed[$0] ?? 0)" }.joined(separator: " ")
            ovpnLog(.info, title: "ConfigStrip", message: summary)
        }
        if !normalized.isEmpty {
            let summary = normalized.keys.sorted().map { "\($0)=\(normalized[$0] ?? 0)" }.joined(separator: " ")
            ovpnLog(.info, title: "ConfigNormalize", message: summary)
        }

        return output.joined(separator: "\n")
    }

    private static func isIPv4Address(_ value: String) -> Bool {
        let parts = value.split(separator: ".")
        if parts.count != 4 { return false }
        for part in parts {
            guard let num = Int(part), num >= 0 && num <= 255 else { return false }
        }
        return true
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

                logOpenVPNConnectionInfo()
                refreshOpenVPNSettingsAfterConnect()
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

    func openVPNAdapterDidReceiveClockTick(_ openVPNAdapter: OpenVPNAdapter) {
        let now = Date()
        if now.timeIntervalSince(lastOpenVPNStatsLogTime) < 5 {
            return
        }
        lastOpenVPNStatsLogTime = now

        let transport = openVPNAdapter.transportStatistics
        let iface = openVPNAdapter.interfaceStatistics
        let transportLine = "transport bytesIn=\(transport.bytesIn) bytesOut=\(transport.bytesOut) packetsIn=\(transport.packetsIn) packetsOut=\(transport.packetsOut)"
        let ifaceLine = "iface bytesIn=\(iface.bytesIn) bytesOut=\(iface.bytesOut) packetsIn=\(iface.packetsIn) packetsOut=\(iface.packetsOut) errorsIn=\(iface.errorsIn) errorsOut=\(iface.errorsOut)"
        ovpnLog(.info, title: "Stats", message: "\(transportLine) | \(ifaceLine)")
    }

    private func refreshOpenVPNSettingsAfterConnect() {
        let localAddr = openVpnLocalAddress
        var net30Gateway: String?
        if let localAddr, let mask = openVpnLocalMask {
            net30Gateway = Self.net30Peer(for: localAddr, mask: mask)
        }
        var gateway = net30Gateway
        if let adapterGateway = ovpnAdapter?.connectionInformation?.gatewayIPv4, !adapterGateway.isEmpty {
            if let localAddr, adapterGateway == localAddr {
                ovpnLog(.info, title: "IPv4Gateway", message: "post-connect ignoring adapter gateway equal to local address=\(adapterGateway)")
            } else if let net30Gateway, net30Gateway != adapterGateway {
                ovpnLog(.info, title: "IPv4Gateway", message: "post-connect keeping net30 peer=\(net30Gateway), adapter gateway=\(adapterGateway)")
            } else {
                gateway = adapterGateway
            }
        }

        guard let gateway, !gateway.isEmpty else { return }
        openVpnGatewayAddress = gateway
        ovpnLog(.info, title: "IPv4Gateway", message: "post-connect gateway=\(gateway)")
    }

}
