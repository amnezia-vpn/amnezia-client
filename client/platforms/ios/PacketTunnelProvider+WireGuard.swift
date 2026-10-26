import Foundation
import NetworkExtension

private let endpointWatchQueue = DispatchQueue(label: Constants.processQueueName + ".endpoint-watch")
private let endpointPollSeconds = 20

extension PacketTunnelProvider {
    func startWireguard(activationAttemptId: String?,
                        errorNotifier: ErrorNotifier,
                        completionHandler: @escaping (Error?) -> Void) {
        guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
              let providerConfiguration = protocolConfiguration.providerConfiguration,
              let wgConfigData: Data = providerConfiguration[Constants.wireGuardConfigKey] as? Data else {
            wg_log(.error, message: "Can't start, config missing")
            completionHandler(nil)
            return
        }

        do {
            let wgConfig = try JSONDecoder().decode(WGConfig.self, from: wgConfigData)
            let wgConfigStr = wgConfig.str

            let tunnelConfiguration = try TunnelConfiguration(fromWgQuickConfig: wgConfigStr)

            if tunnelConfiguration.peers.first!.allowedIPs
                .map({ $0.stringRepresentation })
                .joined(separator: ", ") == "0.0.0.0/0, ::/0" {
                if wgConfig.splitTunnelType == 1 {
                    for index in tunnelConfiguration.peers.indices {
                        tunnelConfiguration.peers[index].allowedIPs.removeAll()
                        var allowedIPs = [IPAddressRange]()

                        for allowedIPString in wgConfig.splitTunnelSites {
                            if let allowedIP = IPAddressRange(from: allowedIPString) {
                                allowedIPs.append(allowedIP)
                            }
                        }

                        tunnelConfiguration.peers[index].allowedIPs = allowedIPs
                    }
                } else if wgConfig.splitTunnelType == 2 {
                    for index in tunnelConfiguration.peers.indices {
                        var excludeIPs = [IPAddressRange]()

                        for excludeIPString in wgConfig.splitTunnelSites {
                            if let excludeIP = IPAddressRange(from: excludeIPString) {
                                excludeIPs.append(excludeIP)
                            }
                        }

                        tunnelConfiguration.peers[index].excludeIPs = excludeIPs
                    }
                }
            }

            wg_log(.info, message: "Starting tunnel from the " +
                   (activationAttemptId == nil ? "OS directly, rather than the app" : "app"))

            // Start the tunnel
            wgTunnelConfiguration = tunnelConfiguration
            wgAdapter = WireGuardAdapter(with: self) { logLevel, message in
                wg_log(logLevel.osLogLevel, message: message)
            }

            wgAdapter?.start(tunnelConfiguration: tunnelConfiguration) { [weak self] adapterError in
                guard let adapterError else {
                    let interfaceName = self?.wgAdapter?.interfaceName ?? "unknown"
                    wg_log(.info, message: "Tunnel interface is \(interfaceName)")
                    self?.startEndpointWatch()
                    completionHandler(nil)
                    return
                }

                switch adapterError {
                case .cannotLocateTunnelFileDescriptor:
                    wg_log(.error, staticMessage: "Starting tunnel failed: could not determine file descriptor")
                    errorNotifier.notify(PacketTunnelProviderError.couldNotDetermineFileDescriptor)
                    completionHandler(PacketTunnelProviderError.couldNotDetermineFileDescriptor)
                case .dnsResolution(let dnsErrors):
                    let hostnamesWithDnsResolutionFailure = dnsErrors.map { $0.address }
                        .joined(separator: ", ")
                    wg_log(.error, message:
                            "DNS resolution failed for the following hostnames: \(hostnamesWithDnsResolutionFailure)")
                    errorNotifier.notify(PacketTunnelProviderError.dnsResolutionFailure)
                    completionHandler(PacketTunnelProviderError.dnsResolutionFailure)
                case .setNetworkSettings(let error):
                    wg_log(.error, message:
                            "Starting tunnel failed with setTunnelNetworkSettings returning \(error.localizedDescription)")
                    errorNotifier.notify(PacketTunnelProviderError.couldNotSetNetworkSettings)
                    completionHandler(PacketTunnelProviderError.couldNotSetNetworkSettings)
                case .startWireGuardBackend(let errorCode):
                    wg_log(.error, message: "Starting tunnel failed with wgTurnOn returning \(errorCode)")
                    errorNotifier.notify(PacketTunnelProviderError.couldNotStartBackend)
                    completionHandler(PacketTunnelProviderError.couldNotStartBackend)
                case .invalidState:
                    fatalError()
                }
            }
        } catch {
            wg_log(.error, message: "Can't parse WG config: \(String(describing: error))")
            errorNotifier.notify(PacketTunnelProviderError.savedProtocolConfigurationIsInvalid)
            completionHandler(PacketTunnelProviderError.savedProtocolConfigurationIsInvalid)
            return
        }
    }

    func handleWireguardStatusMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let completionHandler = completionHandler else { return }
        guard let wgAdapter = wgAdapter else {
            completionHandler(nil)
            return
        }
        wgAdapter.getRuntimeConfiguration { settings in
            guard let settings = settings else {
                completionHandler(nil)
                return
            }
            let components = settings.components(separatedBy: "\n")

            var settingsDictionary: [String: String] = [:]
            for component in components {
                let pair = component.components(separatedBy: "=")
                if pair.count == 2 {
                    settingsDictionary[pair[0]] = pair[1]
                }
            }

            let lastHandshakeString = settingsDictionary["last_handshake_time_sec"]
            let lastHandshake: Int64

            if let lastHandshakeValue = lastHandshakeString, let handshakeValue = Int64(lastHandshakeValue) {
                lastHandshake = handshakeValue
            } else {
                lastHandshake = -2  // Return an error if there is no value for `last_handshake_time_sec`
            }

            let response: [String: Any] = [
                "rx_bytes": settingsDictionary["rx_bytes"] ?? "0",
                "tx_bytes": settingsDictionary["tx_bytes"] ?? "0",
                "last_handshake_time_sec": lastHandshake
            ]

            completionHandler(try? JSONSerialization.data(withJSONObject: response, options: []))
        }
    }

    func handleWireguardAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let completionHandler = completionHandler else { return }
        if messageData.count == 1 && messageData[0] == 0 {
            wgAdapter?.getRuntimeConfiguration { settings in
                var data: Data?
                if let settings {
                    data = settings.data(using: .utf8)!
                }
                completionHandler(data)
            }
        } else if messageData.count >= 1 {
            // Updates the tunnel configuration and responds with the active configuration
            wg_log(.info, message: "Switching tunnel configuration")
            guard let configString = String(data: messageData, encoding: .utf8)
            else {
                completionHandler(nil)
                return
            }

            do {
                let tunnelConfiguration = try TunnelConfiguration(fromWgQuickConfig: configString)
                wgAdapter?.update(tunnelConfiguration: tunnelConfiguration) { [weak self] error in
                    if let error {
                        wg_log(.error, message: "Failed to switch tunnel configuration: \(error.localizedDescription)")
                        completionHandler(nil)
                        return
                    }

                    self?.wgTunnelConfiguration = tunnelConfiguration
                    self?.startEndpointWatch()

                    self?.wgAdapter?.getRuntimeConfiguration { settings in
                        var data: Data?
                        if let settings {
                            data = settings.data(using: .utf8)!
                        }
                        completionHandler(data)
                    }
                }
            } catch {
                completionHandler(nil)
            }
        } else {
            completionHandler(nil)
        }
    }

    func stopWireguard(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        wg_log(.info, message: "Stopping tunnel: reason: \(reason.amneziaDescription)")

        stopEndpointWatch()
        wgAdapter?.stop { error in
            ErrorNotifier.removeLastErrorFile()

            if let error {
                wg_log(.error, message: "Failed to stop WireGuard adapter: \(error.localizedDescription)")
            }
            completionHandler()

#if os(macOS)
            // HACK: This is a filthy hack to work around Apple bug 32073323 (dup'd by us as 47526107).
            // Remove it when they finally fix this upstream and the fix has been rolled out to
            // sufficient quantities of users.
            exit(0)
#endif
        }
    }

    /// Watches the peer while the tunnel is up. When the server stops answering,
    /// resolves its host name again and, if the address changed, applies the
    /// configuration anew. The adapter resolves host names only in start() and
    /// update(); a path update re-resolves the already resolved IP (for NAT64),
    /// not the name, so without this a server that moved to another IP is not
    /// reached again until the tunnel is restarted by hand.
    func startEndpointWatch() {
        stopEndpointWatch()
        guard let endpoint = wgTunnelConfiguration?.peers.first?.endpoint,
              case .name = endpoint.host else { return }

        endpointWatchQueue.async { [weak self] in
            guard let self else { return }
            self.wgEndpointWatchdog.tunnelStarted(nowMs: Self.nowMs())
            // Baseline to compare with: the address the name had at start
            self.wgResolvedEndpoint = Self.resolve(endpoint)
        }

        let timer = DispatchSource.makeTimerSource(queue: endpointWatchQueue)
        timer.schedule(deadline: .now() + .seconds(endpointPollSeconds), repeating: .seconds(endpointPollSeconds))
        timer.setEventHandler { [weak self] in
            self?.checkEndpoint(endpoint)
        }
        wgEndpointTimer = timer
        timer.resume()
    }

    func stopEndpointWatch() {
        wgEndpointTimer?.cancel()
        wgEndpointTimer = nil
    }

    private func checkEndpoint(_ endpoint: Endpoint) {
        wgAdapter?.getRuntimeConfiguration { [weak self] settings in
            guard let settings else { return }
            endpointWatchQueue.async {
                self?.evaluateEndpoint(endpoint, runtimeSettings: settings)
            }
        }
    }

    private func evaluateEndpoint(_ endpoint: Endpoint, runtimeSettings: String) {
        var handshakeSec: Int64 = 0
        var handshakeNsec: Int64 = 0
        var txBytes: UInt64 = 0
        for line in runtimeSettings.split(separator: "\n") {
            let pair = line.split(separator: "=", maxSplits: 1)
            guard pair.count == 2 else { continue }
            switch pair[0] {
            case "last_handshake_time_sec": handshakeSec = Int64(pair[1]) ?? 0
            case "last_handshake_time_nsec": handshakeNsec = Int64(pair[1]) ?? 0
            case "tx_bytes": txBytes = UInt64(pair[1]) ?? 0
            default: break
            }
        }

        let now = Self.nowMs()
        let handshakeMs = handshakeSec * 1000 + handshakeNsec / 1_000_000
        guard wgEndpointWatchdog.observe(nowMs: now, lastHandshakeMs: handshakeMs, txBytes: txBytes) else { return }
        wgEndpointWatchdog.refreshAttempted(nowMs: now)

        let name = endpoint.stringRepresentation
        guard let resolved = Self.resolve(endpoint) else {
            wg_log(.info, message: "No handshake with \(name), resolving it failed")
            return
        }
        guard resolved != wgResolvedEndpoint else {
            wg_log(.info, message: "No handshake with \(name), its address is unchanged")
            return
        }
        guard let tunnelConfiguration = wgTunnelConfiguration else { return }

        wg_log(.info, message: "No handshake with \(name), it moved to \(resolved): applying the configuration again")
        wgResolvedEndpoint = resolved
        wgAdapter?.update(tunnelConfiguration: tunnelConfiguration) { error in
            if let error {
                wg_log(.error, message: "Failed to apply the new server address: \(error.localizedDescription)")
            }
        }
    }

    private static func resolve(_ endpoint: Endpoint) -> String? {
        let results = DNSResolver.resolveSync(endpoints: [endpoint])
        guard let result = results.first ?? nil, case .success(let resolved) = result else { return nil }
        return resolved.stringRepresentation
    }

    private static func nowMs() -> Int64 {
        return Int64(Date().timeIntervalSince1970 * 1000)
    }
}
