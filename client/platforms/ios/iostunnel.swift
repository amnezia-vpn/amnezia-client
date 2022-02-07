// SPDX-License-Identifier: MIT
// Copyright © 2018-2020 WireGuard LLC. All Rights Reserved.

import Foundation
import NetworkExtension
import os
import OpenVPNAdapter

enum TunnelProtoType: String {
    case wireguard, openvpn, none
}

class PacketTunnelProvider: NEPacketTunnelProvider {
    
    private lazy var wgAdapter: WireGuardAdapter = {
        return WireGuardAdapter(with: self) { logLevel, message in
            wg_log(logLevel.osLogLevel, message: message)
        }
    }()
    
    private lazy var ovpnAdapter: OpenVPNAdapter = {
        let adapter = OpenVPNAdapter()
        adapter.delegate = self
        return adapter
    }()
    
    let vpnReachability = OpenVPNReachability()

    var startHandler: ((Error?) -> Void)?
    var stopHandler: (() -> Void)?
    var protoType: TunnelProtoType = .wireguard
    
    override func startTunnel(options: [String: NSObject]?, completionHandler: @escaping (Error?) -> Void) {
        let activationAttemptId = options?["activationAttemptId"] as? String
        let errorNotifier = ErrorNotifier(activationAttemptId: activationAttemptId)
        
        Logger.configureGlobal(tagged: "NET", withFilePath: FileManager.logFileURL?.path)
        
        if let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
           let providerConfiguration = protocolConfiguration.providerConfiguration,
           let _: Data = providerConfiguration["ovpn"] as? Data {
            protoType = .openvpn
        } else {
            protoType = .wireguard
        }
        
        switch protoType {
        case .wireguard:
            startWireguard(activationAttemptId: activationAttemptId,
                           errorNotifier: errorNotifier,
                           completionHandler: completionHandler)
        case .openvpn:
            startOpenVPN(completionHandler: completionHandler)
        case .none:
            break
        }
    }
    
    override func stopTunnel(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        switch protoType {
        case .wireguard:
            stopWireguard(with: reason, completionHandler: completionHandler)
        case .openvpn:
            stopOpenVPN(with: reason, completionHandler: completionHandler)
        case .none:
            break
        }
    }
    
    override func handleAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let completionHandler = completionHandler else { return }
        
        if messageData.count == 1 && messageData[0] == 0 {
            wgAdapter.getRuntimeConfiguration { settings in
                var data: Data?
                if let settings = settings {
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
                wgAdapter.update(tunnelConfiguration: tunnelConfiguration) { error in
                    if let error = error {
                        wg_log(.error, message: "Failed to switch tunnel configuration: \(error.localizedDescription)")
                        completionHandler(nil)
                        return
                    }
                    
                    self.wgAdapter.getRuntimeConfiguration { settings in
                        var data: Data?
                        if let settings = settings {
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
    
    // MARK: Private methods
    private func startWireguard(activationAttemptId: String?,
                                errorNotifier: ErrorNotifier,
                                completionHandler: @escaping (Error?) -> Void) {
        guard let tunnelProviderProtocol = self.protocolConfiguration as? NETunnelProviderProtocol,
              let tunnelConfiguration = tunnelProviderProtocol.asTunnelConfiguration() else {
                  errorNotifier.notify(PacketTunnelProviderError.savedProtocolConfigurationIsInvalid)
                  completionHandler(PacketTunnelProviderError.savedProtocolConfigurationIsInvalid)
                  return
              }
        wg_log(.info, message: "Starting wireguard tunnel from the " + (activationAttemptId == nil ? "OS directly, rather than the app" : "app"))
        
        // Start the tunnel
        wgAdapter.start(tunnelConfiguration: tunnelConfiguration) { adapterError in
            guard let adapterError = adapterError else {
                let interfaceName = self.wgAdapter.interfaceName ?? "unknown"
                
                wg_log(.info, message: "Tunnel interface is \(interfaceName)")
                
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
                wg_log(.error, message: "DNS resolution failed for the following hostnames: \(hostnamesWithDnsResolutionFailure)")
                errorNotifier.notify(PacketTunnelProviderError.dnsResolutionFailure)
                completionHandler(PacketTunnelProviderError.dnsResolutionFailure)
                
            case .setNetworkSettings(let error):
                wg_log(.error, message: "Starting tunnel failed with setTunnelNetworkSettings returning \(error.localizedDescription)")
                errorNotifier.notify(PacketTunnelProviderError.couldNotSetNetworkSettings)
                completionHandler(PacketTunnelProviderError.couldNotSetNetworkSettings)
                
            case .startWireGuardBackend(let errorCode):
                wg_log(.error, message: "Starting tunnel failed with wgTurnOn returning \(errorCode)")
                errorNotifier.notify(PacketTunnelProviderError.couldNotStartBackend)
                completionHandler(PacketTunnelProviderError.couldNotStartBackend)
                
            case .invalidState:
                // Must never happen
                fatalError()
            }
        }
    }
    
    private func startOpenVPN(completionHandler: @escaping (Error?) -> Void) {
        guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
           let providerConfiguration = protocolConfiguration.providerConfiguration,
           let ovpnConfiguration: Data = providerConfiguration["ovpn"] as? Data else {
            return
        }
        
        let configuration = OpenVPNConfiguration()
        configuration.fileContent = ovpnConfiguration
//        configuration.settings = [] // Additional setting if needed any
//        configuration.tunPersist = true // keep tun active during pauses/reconections
        let evaluation: OpenVPNConfigurationEvaluation
        do {
            evaluation = try ovpnAdapter.apply(configuration: configuration)
        } catch {
            completionHandler(error)
            return
        }
        
        if !evaluation.autologin {
            print("Implement login with user credentials")
        }
        
        vpnReachability.startTracking { [weak self] status in
            guard status == .reachableViaWiFi else { return }
            self?.ovpnAdapter.reconnect(afterTimeInterval: 5)
        }
        
        startHandler = completionHandler
        ovpnAdapter.connect(using: packetFlow)
    }
    
    private func stopWireguard(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        wg_log(.info, staticMessage: "Stopping tunnel")
        
        wgAdapter.stop { error in
            ErrorNotifier.removeLastErrorFile()
            
            if let error = error {
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
    
    private func stopOpenVPN(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        stopHandler = completionHandler
        if vpnReachability.isTracking {
            vpnReachability.stopTracking()
        }
        ovpnAdapter.disconnect()
    }
}

extension WireGuardLogLevel {
    var osLogLevel: OSLogType {
        switch self {
        case .verbose:
            return .debug
        case .error:
            return .error
        }
    }
}

extension NEPacketTunnelFlow: OpenVPNAdapterPacketFlow {}

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
        
        // Set the network settings for the current tunneling session.
        setTunnelNetworkSettings(networkSettings, completionHandler: completionHandler)
    }
    
    // Process events returned by the OpenVPN library
    func openVPNAdapter(
        _ openVPNAdapter: OpenVPNAdapter,
        handleEvent event:
        OpenVPNAdapterEvent, message: String?
    ) {
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
        // Handle only fatal errors
        guard let fatal = (error as NSError).userInfo[OpenVPNAdapterErrorFatalKey] as? Bool,
              fatal == true else { return }
        
        if vpnReachability.isTracking {
            vpnReachability.stopTracking()
        }
        
        if let startHandler = startHandler {
            startHandler(error)
            self.startHandler = nil
        } else {
            cancelTunnelWithError(error)
        }
    }
    
    // Use this method to process any log message returned by OpenVPN library.
    func openVPNAdapter(_ openVPNAdapter: OpenVPNAdapter, handleLogMessage logMessage: String) {
        // Handle log messages
    }
}


