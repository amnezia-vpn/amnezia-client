import Foundation
import NetworkExtension
import os
import Darwin
import OpenVPNAdapter
//import Tun2socks
enum TunnelProtoType: String {
    case wireguard, openvpn, shadowsocks, none
}

struct Constants {
    static let kDefaultPathKey = "defaultPath"
    static let processQueueName = "org.amnezia.process-packets"
    static let ssQueueName = "org.amnezia.shadowsocks"
    static let kActivationAttemptId = "activationAttemptId"
    static let ovpnConfigKey = "ovpn"
    static let ssConfigKey = "ss"
    static let loggerTag = "NET"
    static let ssRemoteHost = "server"
    static let ssRemotePort = "server_port"
    static let ssLocalAddressKey = "local_addr"
    static let ssLocalPortKey = "local_port"
    static let ssTimeoutKey = "timeout"
    static let ssCipherKey = "method"
    static let ssPasswordKey = "password"
    static let kActionStart = "start"
    static let kActionRestart = "restart"
    static let kActionStop = "stop"
    static let kActionGetTunnelId = "getTunnelId"
    static let kActionIsServerReachable = "isServerReachable"
    static let kMessageKeyAction = "action"
    static let kMessageKeyTunnelid = "tunnelId"
    static let kMessageKeyConfig = "config"
    static let kMessageKeyErrorCode = "errorCode"
    static let kMessageKeyHost = "host"
    static let kMessageKeyPort = "port"
    static let kMessageKeyOnDemand = "is-on-demand"
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
    
    private var shadowSocksConfig: Data? = nil
    private var openVPNConfig: Data? = nil
    var ssCompletion: ShadowsocksProxyCompletion = nil
    
//    private var ssProvider: ShadowSocksTunnel? = nil
//    private var ssLocalPort: Int = 8585
//    private var ssRemoteHost = ""
//    private var leafProvider: TunProvider? = nil
//
//    private var tun2socksTunnel: Tun2socksOutlineTunnelProtocol? = nil
//    private var tun2socksWriter: Tun2socksTunWriter? = nil
//    private let processQueue = DispatchQueue(label: Constants.processQueueName)
//    private var connection: NWTCPConnection? = nil
//    private var session: NWUDPSession? = nil
//    private var observer: AnyObject?
   
    let vpnReachability = OpenVPNReachability()

    var startHandler: ((Error?) -> Void)?
    var stopHandler: (() -> Void)?
    var protoType: TunnelProtoType = .wireguard
    
    override func startTunnel(options: [String: NSObject]?, completionHandler: @escaping (Error?) -> Void) {
        let activationAttemptId = options?[Constants.kActivationAttemptId] as? String
        let errorNotifier = ErrorNotifier(activationAttemptId: activationAttemptId)
        
        Logger.configureGlobal(tagged: Constants.loggerTag, withFilePath: FileManager.logFileURL?.path)
        
        if let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
           let providerConfiguration = protocolConfiguration.providerConfiguration,
           let _: Data = providerConfiguration[Constants.ovpnConfigKey] as? Data {
            let withoutShadowSocks = providerConfiguration[Constants.ssConfigKey] as? Data == nil
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
            break
//            startShadowSocks(completionHandler: completionHandler)
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
            break
//            stopShadowSocks(with: reason, completionHandler: completionHandler)
        case .none:
            break
        }
    }
    
    override func handleAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        switch protoType {
        case .wireguard:
            handleWireguardAppMessage(messageData, completionHandler: completionHandler)
        case .openvpn:
            handleOpenVPNAppMessage(messageData, completionHandler: completionHandler)
        case .shadowsocks:
            break
//            handleShadowSocksAppMessage(messageData, completionHandler: completionHandler)
        case .none:
            break
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
              let ovpnConfiguration: Data = providerConfiguration[Constants.ovpnConfigKey] as? Data else {
            // TODO: handle errors properly
               wg_log(.error, message: "Can't start startOpenVPN()")
            return
        }
        
        setupAndlaunchOpenVPN(withConfig: ovpnConfiguration, completionHandler: completionHandler)
    }
/*
    private func startShadowSocks(completionHandler: @escaping (Error?) -> Void) {
        guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
              let providerConfiguration = protocolConfiguration.providerConfiguration,
              let ssConfiguration: Data = providerConfiguration[Constants.ssConfigKey] as? Data,
              let ovpnConfiguration: Data = providerConfiguration[Constants.ovpnConfigKey] as? Data else {
                  // TODO: handle errors properly
                  wg_log(.error, message: "Cannot start startShadowSocks()")
                  return
              }
        self.shadowSocksConfig = ssConfiguration
        self.openVPNConfig = ovpnConfiguration
        wg_log(.info, message: "Prepare to start shadowsocks/tun2socks/leaf")
//        self.startSSProvider(completion: completionHandler)
//        startTun2SocksTunnel(completion: completionHandler)
        self.startLeafRedirector(completion: completionHandler)
    }
*/
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
/*
    private func stopShadowSocks(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        stopOpenVPN(with: reason) { [weak self] in
            guard let `self` = self else { return }
//            self.stopSSProvider(completionHandler: completionHandler)
//            self.stopTun2SocksTunnel(completionHandler: completionHandler)
            self.stopLeafRedirector(completion: completionHandler)
        }
    }
*/
    private func handleWireguardAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
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
    
    private func handleOpenVPNAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let completionHandler = completionHandler else { return }
        if messageData.count == 1 && messageData[0] == 0 {
            let bytesin = ovpnAdapter.transportStatistics.bytesIn
            let strBytesin = "rx_bytes=" + String(bytesin);
            
            let bytesout = ovpnAdapter.transportStatistics.bytesOut
            let strBytesout = "tx_bytes=" + String(bytesout);
            
            let strData = strBytesin + "\n" + strBytesout;
            let data = Data(strData.utf8)
            completionHandler(data)
    }
    }
    
/*
    private func handleShadowSocksAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let completionHandler = completionHandler else { return }
        if let configString = String(data: messageData, encoding: .utf8) {
            wg_log(.debug, message: configString)
        }
        
        completionHandler(messageData)
    }
*/
    // MARK: -- Tun2sock provider methods
/*
    private func startTun2SocksTunnel(completion: @escaping (Error?) -> Void) {
        guard let ssConfiguration = self.shadowSocksConfig,
              let ovpnConfiguration = self.openVPNConfig,
              let ssConfig = try? JSONSerialization.jsonObject(with: ssConfiguration, options: []) as? [String: Any]
        else {
            wg_log(.info, message: "Cannot parse shadowsocks config")
            let tun2socksError: NSError = .init(domain: "", code: 100, userInfo: nil)
            completion(tun2socksError)
            return
        }
        wg_log(.info, message: "SS Config: \(ssConfig)")
        
        guard let remoteHost = ssConfig[Constants.ssRemoteHost] as? String,
              let remotePort = ssConfig[Constants.ssRemotePort] as? Int,
              let method = ssConfig[Constants.ssCipherKey] as? String,
              let password = ssConfig[Constants.ssPasswordKey] as? String else {
                  wg_log(.error, message: "Cannot parse ss config")
                  let tun2socksError: NSError = .init(domain: "", code: 100, userInfo: nil)
                  completion(tun2socksError)
                  return
              }
        
        let connError: AutoreleasingUnsafeMutablePointer<NSError?>? = nil
        ShadowsocksCheckConnectivity(remoteHost, remotePort, password, method, nil, connError)
        if (connError?.pointee != nil) {
            wg_log(.error, message: "Failed to start tun2socks tunnel with error: \(connError?.pointee?.localizedDescription ?? "oops")")
            let tun2socksError: NSError = .init(domain: "", code: 100, userInfo: nil)
            completion(tun2socksError)
            return
        }
        setupAndlaunchOpenVPN(withConfig: ovpnConfiguration, withShadowSocks: true) { vpnError in
            guard vpnError == nil else {
                wg_log(.error, message: "Failed to start openvpn with tun2socks tunnel with error: \(vpnError?.localizedDescription ?? "oops")")
                let tun2socksError: NSError = .init(domain: "", code: 100, userInfo: nil)
                completion(tun2socksError)
                return
            }
//        let ipv4settings: NEIPv4Settings = .init(addresses: ["192.0.2.1"], subnetMasks: ["255.255.255.0"])
//        ipv4settings.includedRoutes = [.default()]
//        ipv4settings.excludedRoutes = []
//
//        let dnsSettings: NEDNSSettings = .init(servers: ["1.1.1.1", "9.9.9.9", "208.67.222.222", "208.67.220.220"])
//        let settings: NEPacketTunnelNetworkSettings = .init(tunnelRemoteAddress: "192.0.2.2")
//        settings.ipv4Settings = ipv4settings
//        settings.dnsSettings = dnsSettings
//        settings.mtu = 1600
//
//        setTunnelNetworkSettings(settings) { tunError in
            let ifaces = Interface.allInterfaces()
                .filter { $0.family == .ipv4 }
                .map { iface in iface.name }
            
            wg_log(.error, message: "Available TUN Interfaces: \(ifaces)")
            self.tun2socksWriter = Tun2socksTunWriter()
            let tunError: AutoreleasingUnsafeMutablePointer<NSError?>? = nil
            self.tun2socksTunnel = Tun2socksConnectShadowsocksTunnel(self.tun2socksWriter, remoteHost, remotePort, password, method, false, tunError)
            if (tunError?.pointee != nil) {
                wg_log(.error, message: "Failed to start tun2socks tunnel with error: \(tunError?.pointee?.localizedDescription ?? "oops")")
                let tun2socksError: NSError = .init(domain: "", code: 100, userInfo: nil)
                completion(tun2socksError)
                return
            }
            self.processQueue.async { self.processPackets() }
            completion(nil)
        }
    }
    
    private func stopTun2SocksTunnel(completionHandler: @escaping () -> Void) {
        if self.tun2socksTunnel != nil && self.tun2socksTunnel!.isConnected() {
            self.tun2socksTunnel?.disconnect()
        }
        try? self.tun2socksWriter?.close()
        completionHandler()
    }
    
    private func processPackets() {
        packetFlow.readPacketObjects { [weak self] packets in
            guard let `self` = self else { return }
            do {
                let _ = try packets.map {
                    var bytesWritten: Int = 0
                    try self.tun2socksTunnel?.write($0.data, ret0_: &bytesWritten)
                    self.processQueue.async {
                        self.processPackets()
                    }
                }
            } catch (let err) {
                wg_log(.debug, message: "Error in tun2sock: \(err.localizedDescription)")
            }
        }
    }
    // MARK: -- Leaf provider methods
    private func prepareConfig(onInterface iface: String, fromSSConfig ssConfig: Data, andOvpnConfig ovpnConfig: Data) -> UnsafePointer<CChar>? {
        guard let ssConfig = try? JSONSerialization.jsonObject(with: ssConfig, options: []) as? [String: Any] else {
        self.ssCompletion?(0, NSError(domain: Bundle.main.bundleIdentifier ?? "unknown",
                                 code: 100,
                                 userInfo: [NSLocalizedDescriptionKey: "Cannot parse json for ss in tunnel"]))
            return nil
        }
        guard let remoteHost = ssConfig[Constants.ssRemoteHost] as? String,
              let remotePort = ssConfig[Constants.ssRemotePort] as? Int,
              let method = ssConfig[Constants.ssCipherKey] as? String,
              let password = ssConfig[Constants.ssPasswordKey] as? String else {
                  self.ssCompletion?(0, NSError(domain: Bundle.main.bundleIdentifier ?? "unknown",
                                           code: 100,
                                           userInfo: [NSLocalizedDescriptionKey: "Cannot assign profile params for ss in tunnel"]))
                  return nil
              }
        var insettings:  [String: Any] = .init()
        insettings["name"] = iface
        insettings["address"] = "127.0.0.2"
        insettings["netmask"] = "255.255.255.0"
        insettings["gateway"] = "127.0.0.1"
        insettings["mtu"] = 1600
        var inbounds: [String: Any] = .init()
        inbounds["protocol"] = "tun"
        inbounds["settings"] = insettings
        inbounds["tag"] = "tun_in"
        var outbounds: [String: Any] = .init()
        var outsettings: [String: Any] = .init()
        outsettings["address"] = remoteHost
        outsettings["port"] = remotePort
        outsettings["method"] = method
        outsettings["password"] = password
        outbounds["protocol"] = "shadowsocks"
        outbounds["settings"] = outsettings
        outbounds["tag"] = "shadowsocks_out"
        var params: [String: Any] = .init()
        params["inbounds"] = [inbounds]
        params["outbounds"] =  [outbounds]
        wg_log(.error, message: "Config dictionary: \(params)")
        guard let jsonData = try? JSONSerialization.data(withJSONObject: params, options: .prettyPrinted),
              let jsonString = String(data: jsonData, encoding: .utf8) else { return nil }
        wg_log(.error, message: "JSON String: \(jsonString)")
        var path = ""
        if let documentDirectory = FileManager.default.urls(for: .documentDirectory,
                                                               in: .userDomainMask).first {
            let pathWithFilename = documentDirectory.appendingPathComponent("config.json")
            do {
                try jsonString.write(to: pathWithFilename,
                                     atomically: true,
                                     encoding: .utf8)
                path = pathWithFilename.path
            } catch {
                // Handle error
            }
        }
        
        return UnsafePointer(strdup(path))
    }
    
    private func startLeafRedirector(completion: @escaping (Error?) -> Void) {
        let ipv4settings: NEIPv4Settings = .init(addresses: ["127.0.0.2"], subnetMasks: ["255.255.255.0"])
        ipv4settings.includedRoutes = [.default()]
        ipv4settings.excludedRoutes = []
        
        let dnsSettings: NEDNSSettings = .init(servers: ["1.1.1.1", "9.9.9.9", "208.67.222.222", "208.67.220.220"])
        dnsSettings.matchDomains = []
        let settings: NEPacketTunnelNetworkSettings = .init(tunnelRemoteAddress: "127.0.0.1")
        settings.ipv4Settings = ipv4settings
        settings.dnsSettings = dnsSettings
        settings.mtu = 1600
        
        self.setTunnelNetworkSettings(settings) { tunError in
            let ifaces = Interface.allInterfaces()
                .filter { $0.name.contains("tun") && $0.family == .ipv4 }
                .map { iface in iface.name }
            wg_log(.error, message: "Try on interface: \(ifaces)")
            guard let ssConf = self.shadowSocksConfig,
                  let ovpnConf = self.openVPNConfig,
                  let config = self.prepareConfig(onInterface: ifaces.first ?? "utun2",
                                                  fromSSConfig: ssConf,
                                                  andOvpnConfig: ovpnConf) else {
                      let ret: NSError = .init(domain: "", code: 100, userInfo: nil)
                      completion(ret)
                      return
                  }
            self.leafProvider = TunProvider(withConfig: config)
            self.leafProvider?.testConfig(onPath: config) { configError in
                wg_log(.error, message: "Config check status: \(configError!.desc)")
                guard configError! == .noError else {
                    wg_log(.error, message: "Config check status: \(configError!.desc)")
                    let ret: NSError = .init(domain: "", code: 100, userInfo: nil)
                    completion(ret)
                    return
                }
                
                wg_log(.error, message: "Available TUN Interfaces: \(ifaces)")
               
                self.leafProvider?.startTunnel { tunError in
                    wg_log(.error, message: "Leaf tunnel start status: \(tunError!.desc)")
                    guard tunError! == .noError else {
                        wg_log(.error, message: "Leaf tunnel start error: \(tunError!.desc)")
                        let ret: NSError = .init(domain: "", code: 100, userInfo: nil)
                        completion(ret)
                        return
                    }
                    completion(nil)
                }
            }
            
        }
    }
    private func stopLeafRedirector(completion: @escaping () -> Void) {
        leafProvider?.stopTunnel { error in
            // TODO: handle errors
            completion()
        }
    }
    
    // MARK: -- ShadowSocks Provider methods
    
    private func startSSProvider(completion: @escaping (Error?) -> Void) {
        guard let ssConfig = self.shadowSocksConfig, let ovpnConfig = self.openVPNConfig else { return }
        if ssProvider == nil {
            guard let config = try? JSONSerialization.jsonObject(with: ssConfig, options: []) as? [String: Any],
                  let remoteHost = config[Constants.ssRemoteHost] as? String,
                  let port = config[Constants.ssLocalPortKey] as? Int else {
            self.ssCompletion?(0, NSError(domain: Bundle.main.bundleIdentifier ?? "unknown",
                                     code: 100,
                                     userInfo: [NSLocalizedDescriptionKey: "Cannot parse json for ss in tunnel"]))
                return
            }
            ssProvider = SSProvider(config: ssConfig, localPort: port)
            ssLocalPort = port
            ssRemoteHost = remoteHost
        }
        
        ssProvider?.start(usingPacketFlow: packetFlow, withConnectivityCheck: false) { errorCode in
            wg_log(.info, message: "After starting shadowsocks")
            wg_log(.error, message: "Starting ShadowSocks State: \(String(describing: errorCode))")
            if (errorCode != nil && errorCode! != .noError) {
                wg_log(.error, message: "Error starting ShadowSocks: \(String(describing: errorCode))")
                return
            }
//            self.setupAndHandleOpenVPNOverSSConnection(withConfig: ovpnConfig)
            self.startAndHandleTunnelOverSS(completionHandler: completion)
        }
    }
    
    private func startAndHandleTunnelOverSS(completionHandler: @escaping (Error?) -> Void) {
//        let ipv4settings: NEIPv4Settings = .init(addresses: ["192.0.2.2"], subnetMasks: ["255.255.255.0"])
//        let addedRoute1 = NEIPv4Route(destinationAddress: "0.0.0.0", subnetMask: "0.0.0.0")
//        addedRoute1.gatewayAddress = "192.0.2.1"
//        ipv4settings.includedRoutes = [addedRoute1]
//        ipv4settings.excludedRoutes = []
//
//        let dnsSettings: NEDNSSettings = .init(servers: ["1.1.1.1", "9.9.9.9", "208.67.222.222", "208.67.220.220"])
//        let settings: NEPacketTunnelNetworkSettings = .init(tunnelRemoteAddress: "192.0.2.1")
//        settings.ipv4Settings = ipv4settings
//        settings.dnsSettings = dnsSettings
//        settings.mtu = 1600
//
//        setTunnelNetworkSettings(settings) { tunError in
//
//        }
        
        let ifaces = Interface.allInterfaces()
            .filter { $0.family == .ipv4 }
            .map { iface in iface.name }
        
        wg_log(.error, message: "Available TUN Interfaces: \(ifaces)")
        let endpoint = NWHostEndpoint(hostname: "127.0.0.1", port: "\(self.ssLocalPort)")
        self.session = self.createUDPSession(to: endpoint, from: nil)
        self.setupWriteToFlow()
        self.observer = self.session!.observe(\.state, options: [.new]) { conn, _  in
            switch conn.state {
            case .ready:
                self.readFromFlow()
                completionHandler(nil)
            case .cancelled, .failed, .invalid:
                self.stopSSProvider {
                    self.cancelTunnelWithError(nil)
                    completionHandler(nil)
                }
            default:
                break
            }
        }
    }
    
    private func setupAndHandleOpenVPNOverSSConnection(withConfig ovpnConfig: Data) {
        let endpoint = NWHostEndpoint(hostname: "127.0.0.1", port: "\(self.ssLocalPort)")
        self.session = self.createUDPSession(to: endpoint, from: nil)
//        self.connection = self.createTCPConnection(to: endpoint, enableTLS: false, tlsParameters: nil, delegate: nil)
        self.observer = self.session!.observe(\.state, options: [.new]) { conn, _  in
            switch conn.state {
            case .ready:
                self.processQueue.async {
                    self.setupWriteToFlow()
                }
                self.processQueue.async {
                    self.readFromFlow()
                }
                
                self.setupAndlaunchOpenVPN(withConfig: ovpnConfig, withShadowSocks: true) { vpnError in
                    wg_log(.info, message: "After starting openVPN")
                    guard vpnError == nil else {
                        wg_log(.error, message: "Failed to start openvpn with error: \(vpnError?.localizedDescription ?? "oops")")
                        return
                    }
                }
            case .cancelled, .failed, .invalid:
                self.stopSSProvider {
                    self.cancelTunnelWithError(nil)
                }
            default:
                break
            }
        }
    }
    
    private func readFromFlow() {
        wg_log(.error, message: "Start reading packets to connection")
        wg_log(.error, message: "Connection is \(session != nil ? "not null" : "null")")
        packetFlow.readPackets { [weak self] packets, protocols in
            wg_log(.error, message: "\(packets.count) outcoming packets processed of \(protocols.first?.stringValue ?? "unknown") type")
            guard let `self` = self else { return }
            self.session?.writeMultipleDatagrams(packets, completionHandler: { _ in
                self.processQueue.async {
                    self.readFromFlow()
                }
            })
//            let _ = packets.map {
//                wg_log(.error, message: "Packet: \($0.data) of \($0.protocolFamily)")
//                self.connection?.write($0.data, completionHandler: { _ in })
//                self.processQueue.async {
//                    self.readFromFlow()
//                }
//            }
        }
    }
    
    private func setupWriteToFlow() {
        wg_log(.error, message: "Start writing packets from connection")
        wg_log(.error, message: "Connection is \(session != nil ? "not null" : "null")")
        session?.setReadHandler({ ssdata, error in
            wg_log(.error, message: "Packets are \(ssdata != nil ? "not null" : "null"), error: \(error?.localizedDescription ?? "none")")
            guard error == nil, let packets = ssdata else { return }
            wg_log(.error, message: "\(packets.count) incoming packets processed")
            self.packetFlow.writePackets(packets, withProtocols: [NSNumber(value: AF_INET)])
        }, maxDatagrams: Int.max)
        
//        connection?.readLength(1450, completionHandler: { [weak self] ssdata, readError in
//            wg_log(.error, message: "Packets are \(ssdata != nil ? "not null" : "null")")
//            guard let `self` = self, let packets = ssdata else { return }
//            wg_log(.error, message: "Packet: \(packets) or error: \(readError?.localizedDescription ?? "")")
//            self.packetFlow.writePackets([packets], withProtocols: [NSNumber(value: AF_INET)])
//            self.processQueue.async {
//                self.writeToFlow()
//            }
//        })
    }
    
    private func stopSSProvider(completionHandler: @escaping () -> Void) {
        self.ssProvider?.stop { _ in
            if let provider = self.ssProvider, let threadId = provider.ssLocalThreadId {
                pthread_kill(threadId, SIGUSR1)
            }
            self.ssProvider = nil
            completionHandler()
        }
    }
*/
    private func setupAndlaunchOpenVPN(withConfig ovpnConfiguration: Data, withShadowSocks viaSS: Bool = false, completionHandler: @escaping (Error?) -> Void) {
        wg_log(.info, message: "Inside setupAndlaunchOpenVPN()")
        let str = String(decoding: ovpnConfiguration, as: UTF8.self)
        wg_log(.info, message: "OPENVPN config: \(str)")
        
        let configuration = OpenVPNConfiguration()
        configuration.fileContent = ovpnConfiguration
        if viaSS {
//            configuration.settings = [
//                "remote": "137.74.6.148 1194",
//                "proto": "tcp",
//                "link-mtu": "1480",
//                "tun-mtu": "1460",
//            ]
        }
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
    
//        let ifaces = Interface.allInterfaces()
//            .filter { $0.family == .ipv4 }
//            .map { iface in iface.name }
        
//        wg_log(.error, message: "Available TUN Interfaces: \(ifaces)")
    }
    
    // MARK: -- Network observing methods
    
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
    
    private func startListeningForNetworkChanges() {
        stopListeningForNetworkChanges()
        addObserver(self, forKeyPath: Constants.kDefaultPathKey, options: .old, context: nil)
    }

    private func stopListeningForNetworkChanges() {
        removeObserver(self, forKeyPath: Constants.kDefaultPathKey)
    }

    override func observeValue(forKeyPath keyPath: String?,
                                     of object: Any?,
                                     change: [NSKeyValueChangeKey : Any]?,
                                     context: UnsafeMutableRawPointer?) {
        guard Constants.kDefaultPathKey != keyPath else { return }
        // Since iOS 11, we have observed that this KVO event fires repeatedly when connecting over Wifi,
        // even though the underlying network has not changed (i.e. `isEqualToPath` returns false),
        // leading to "wakeup crashes" due to excessive network activity. Guard against false positives by
        // comparing the paths' string description, which includes properties not exposed by the class
        guard let lastPath: NWPath = change?[.oldKey] as? NWPath,
              let defPath = defaultPath,
              lastPath != defPath || lastPath.description != defPath.description else {
                  return
              }
        DispatchQueue.main.async { [weak self] in
            guard let `self` = self, self.defaultPath != nil else { return }
            self.handle(networkChange: self.defaultPath!) { _ in }
        }
    }

    private func handle(networkChange changePath: NWPath, completion: @escaping (Error?) -> Void) {
        wg_log(.info, message: "Tunnel restarted.")
        startTunnel(options: nil, completionHandler: completion)
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

/* extension NEPacketTunnelFlow: ShadowSocksAdapterPacketFlow {} */

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
/*
extension PacketTunnelProvider: Tun2socksTunWriterProtocol {
    func write(_ p0: Data?, n: UnsafeMutablePointer<Int>?) throws {
        if let packets = p0 {
            self.packetFlow.writePackets([packets], withProtocols: [NSNumber(value: AF_INET)])
        }
    }
    
    func close() throws {}
}
*/
