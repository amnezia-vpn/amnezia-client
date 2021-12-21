// SPDX-License-Identifier: MIT
// Copyright © 2018-2020 WireGuard LLC. All Rights Reserved.

import Foundation
import NetworkExtension
import os
import OpenVPNAdapter
import ShadowSocks
//import Tun2Socks

enum TunnelProtoType: String {
    case wireguard, openvpn, shadowsocks, none
}

typealias ShadowsocksProxyCompletion = ((Int32, NSError?) -> Void)?

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
    
    private var shadowSocksPort: Int32 = 0
    private var isShadowsocksRunning: Bool = false
    var ssCompletion: ShadowsocksProxyCompletion = nil
    private let ssQueue = DispatchQueue(label: "org.amnezia.shadowsocks")
    private var shadowSocksConfig: Data? = nil
    
//    private var tun2socksWriter: AmneziaTun2SocksWriter? = nil
//    private var tun2socksTunnel: Tun2socksOutlineTunnelProtocol? = nil
//    private let processQueue = DispatchQueue(label: "org.amnezia.process-packets")
   
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
            let withoutShadowSocks = providerConfiguration["ss"] as? Data == nil
            protoType = withoutShadowSocks ? .openvpn : .shadowsocks
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
        case .shadowsocks:
            startShadowSocks { error in
                
            }
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
        case .shadowsocks:
            stopShadowSocks(with: reason, completionHandler: completionHandler)
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
            // TODO: handle errors properly
               wg_log(.error, message: "Can't start startOpenVPN()")
            return
        }
        
        setupAndlaunchOpenVPN(withConfig: ovpnConfiguration, completionHandler: completionHandler)
    }
    
    private func startShadowSocks(completionHandler: @escaping (Error?) -> Void) {
        guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
              let providerConfiguration = protocolConfiguration.providerConfiguration,
              let ssConfiguration: Data = providerConfiguration["ss"] as? Data,
              let ovpnConfiguration: Data = providerConfiguration["ovpn"] as? Data else {
                  // TODO: handle errors properly
                  wg_log(.error, message: "Cannot start startShadowSocks()")
                  return
              }
//        self.shadowSocksConfig = ssConfiguration
//
//        guard let config = self.shadowSocksConfig else { return }
//        guard let ssConfig = try? JSONSerialization.jsonObject(with: config, options: []) as? [String: Any] else {
//            self.ssCompletion?(0, NSError(domain: Bundle.main.bundleIdentifier ?? "unknown",
//                                     code: 100,
//                                     userInfo: [NSLocalizedDescriptionKey: "Cannot parse json for ss in tunnel"]))
//            return
//        }
        
//        let sshost = ssConfig["local_addr"] as? String
//        let ssport = ssConfig["local_port"] as? Int ?? Int(self.shadowSocksPort)
//
//        let method = ssConfig["method"] as? String
//        let password = ssConfig["password"] as? String
        
        
//        Thread.detachNewThread { [weak self] in
        setupAndLaunchShadowSocksProxy(withConfig: ssConfiguration, ssHandler: { [weak self] port, error in
            wg_log(.info,
                   message: "Prepare to start openvpn, self is \(self == nil ? "null" : "not null")")
            guard error == nil else {
                wg_log(.error, message: "Stopping tunnel: \(error?.localizedDescription ?? "none")")
                completionHandler(error!)
                return
            }
            
            self?.setupAndlaunchOpenVPN(withConfig: ovpnConfiguration) { error in
                guard error == nil else {
                    wg_log(.error, message: "Start OpenVPN tunnel error : \(error?.localizedDescription ?? "none")")
                    completionHandler(error!)
                    return
                }
                wg_log(.error, message: "OpenVPN tunnel connected.")
            }
            
//            self?.startTun2Socks(host: sshost, port: ssport, password: password, cipher: method, isUDPEnabled: false, error: nil)
        })
//        }
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
    
    private func stopShadowSocks(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        stopOpenVPN(with: reason) {
//            if self.tun2socksTunnel != nil && self.tun2socksTunnel!.isConnected() {
//                self.tun2socksTunnel?.disconnect()
//                try? self.tun2socksWriter?.close()
//            }
        }
    }
    
//    private func startTun2Socks(host: String?, port: Int, password: String?, cipher: String?, isUDPEnabled: Bool, error: NSErrorPointer) {
//        let isOn = self.tun2socksTunnel != nil && self.tun2socksTunnel!.isConnected()
//        if isOn { tun2socksTunnel?.disconnect() }
//        self.tun2socksWriter = AmneziaTun2SocksWriter(tunnelFlow: self.packetFlow)
//        self.tun2socksTunnel = Tun2socksConnectShadowsocksTunnel(self.tun2socksWriter, host, port, password, cipher, isUDPEnabled, error)
//        if (!isOn) {
//            self.processQueue.sync { self.processPackets() }
//        }
//    }
//
//    private func processPackets() {
//        wg_log(.info, message: "Inside startTun2SocksPacketForwarder")
//        packetFlow.readPacketObjects { [weak self] packets in
//            guard let `self` = self else { return }
//            do {
//                let _ = try packets.map {
//                    var bytesWritten: Int = 0
//                    try self.tun2socksTunnel?.write($0.data, ret0_: &bytesWritten)
//                    self.processQueue.sync {
//                        self.processPackets()
//                    }
//                }
//            } catch (let err) {
//                wg_log(.debug, message: "Error in tun2sock: \(err.localizedDescription)")
//            }
//        }
//    }
    
    private func setupAndlaunchOpenVPN(withConfig ovpnConfiguration: Data, completionHandler: @escaping (Error?) -> Void) {
        wg_log(.info, message: "Inside setupAndlaunchOpenVPN()")
        let str = String(decoding: ovpnConfiguration, as: UTF8.self)
        wg_log(.info, message: "OPENVPN config: \(str)")
        
        let configuration = OpenVPNConfiguration()
        configuration.fileContent = ovpnConfiguration
        let evaluation: OpenVPNConfigurationEvaluation
        do {
            evaluation = try ovpnAdapter.apply(configuration: configuration)
        } catch {
            completionHandler(error)
            return
        }
        
        if !evaluation.autologin {
            wg_log(.info, message: "Implement login with user credentials")
        }
        
        vpnReachability.startTracking { [weak self] status in
            guard status == .reachableViaWiFi else { return }
            self?.ovpnAdapter.reconnect(afterTimeInterval: 5)
        }
        
        startHandler = completionHandler
        ovpnAdapter.connect(using: packetFlow)
    }
    
    private func setupAndLaunchShadowSocksProxy(withConfig config: Data, ssHandler: ShadowsocksProxyCompletion) {
        let str = String(decoding: config, as: UTF8.self)
        wg_log(.info, message: "config: \(str)")
        ssCompletion = ssHandler
        guard let ssConfig = try? JSONSerialization.jsonObject(with: config, options: []) as? [String: Any] else {
            ssHandler?(0, NSError(domain: Bundle.main.bundleIdentifier ?? "unknown",
                                 code: 100,
                                 userInfo: [NSLocalizedDescriptionKey: "Cannot parse json for ss in tunnel"]))
            return
        }
        
        wg_log(.info, message: "SS Config: \(ssConfig)")
        
        guard let remoteHost = ssConfig["server"] as? String, // UnsafeMutablePointer<CChar>,
              let remotePort = ssConfig["server_port"] as? Int32,
              let localAddress = ssConfig["local_addr"] as? String, //UnsafeMutablePointer<CChar>,
              let localPort = ssConfig["local_port"] as? Int32,
              let method = ssConfig["method"] as? String, //UnsafeMutablePointer<CChar>,
              let password = ssConfig["password"] as? String,//UnsafeMutablePointer<CChar>,
              let timeout = ssConfig["timeout"] as? Int32
        else {
            ssHandler?(0, NSError(domain: Bundle.main.bundleIdentifier ?? "unknown",
                                 code: 100,
                                 userInfo: [NSLocalizedDescriptionKey: "Cannot assing profile params for ss in tunnel"]))
            return
        }
        
        /* An example profile
         *
         * const profile_t EXAMPLE_PROFILE = {
         *  .remote_host = "example.com",
         *  .local_addr = "127.0.0.1",
         *  .method = "bf-cfb",
         *  .password = "barfoo!",
         *  .remote_port = 8338,
         *  .local_port = 1080,
         *  .timeout = 600;
         *  .acl = NULL,
         *  .log = NULL,
         *  .fast_open = 0,
         *  .mode = 0,
         *  .verbose = 0
         * };
         */
        
        var profile: profile_t = .init()
        memset(&profile, 0, MemoryLayout<profile_t>.size)
        profile.remote_host = strdup(remoteHost)
        profile.remote_port = remotePort
        profile.local_addr = strdup(localAddress)
        profile.local_port = localPort
        profile.method = strdup(method)
        profile.password = strdup(password)
        profile.timeout = timeout
        profile.acl = nil
        profile.log = nil
        profile.mtu = 1600
        profile.fast_open = 1
        profile.mode = 0
        profile.verbose = 1
        
        wg_log(.debug, message: "Prepare to start shadowsocks proxy server...")
        let observer = UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque())
        ssQueue.sync { [weak self] in
            let success = start_ss_local_server_with_callback(profile, { socks_fd,  udp_fd, data in
                wg_log(.debug, message: "Inside cb callback")
                wg_log(.debug, message: "Params: socks_fd -> \(socks_fd), udp_fd -> \(udp_fd)")
                if let obs = data {
                    wg_log(.debug, message: "Prepare to call onShadowsocksCallback() with socks port \(socks_fd) and udp port \(udp_fd)")
                    let mySelf = Unmanaged<PacketTunnelProvider>.fromOpaque(obs).takeUnretainedValue()
                    mySelf.onShadowsocksCallback(fd: socks_fd)
                }
            }, observer)
            if success != -1 {
                wg_log(.error, message: "ss proxy started on port \(localPort)")
                self?.shadowSocksPort = localPort
                self?.isShadowsocksRunning = true
            } else {
                wg_log(.error, message: "Failed to start ss proxy")
            }
        }
    }
    
    private func onShadowsocksCallback(fd: Int32) {
        wg_log(.debug, message: "Inside onShadowsocksCallback() with port \(fd)")
        var error: NSError? = nil
        if fd > 0 {
//            shadowSocksPort = getSockPort(for: fd)
            isShadowsocksRunning = true
        } else {
            error = NSError(domain: Bundle.main.bundleIdentifier ?? "unknown", code: 100, userInfo: [NSLocalizedDescriptionKey : "Failed to start shadowsocks proxy"])
        }
        ssCompletion?(shadowSocksPort, error)
    }
    
    private func getSockPort(for fd: Int32) -> Int32 {
        var addr_in = sockaddr_in();
        addr_in.sin_len = UInt8(MemoryLayout.size(ofValue: addr_in));
        addr_in.sin_family = sa_family_t(AF_INET);
        
        var len = socklen_t(addr_in.sin_len);
        let result = withUnsafeMutablePointer(to: &addr_in, {
            $0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
                return Darwin.getsockname(fd, $0, &len);
            }
        });
        
        if result == 0 {
            return Int32(addr_in.sin_port);
        } else {
            wg_log(.error, message: "getSockPort(\(fd)) error: \(String(describing: strerror(errno)))")
            return 0
        }
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
        wg_log(.info, message: logMessage)
    }
}

//class AmneziaTun2SocksWriter: Tun2socksTunWriter {
//    private var tunnelFlow: NEPacketTunnelFlow
//
//    init(tunnelFlow: NEPacketTunnelFlow) {
//        self.tunnelFlow = tunnelFlow
//        super.init()
//    }
//
//    override func write(_ p0: Data?, n: UnsafeMutablePointer<Int>?) throws {
//        if let packets = p0 {
//            tunnelFlow.writePackets([packets], withProtocols: [NSNumber(value: AF_INET)])
//        }
//    }
//
//    override func close() throws {}
//}
