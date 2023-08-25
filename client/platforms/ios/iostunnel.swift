import Foundation
import NetworkExtension
import os
import Darwin
import OpenVPNAdapter

enum TunnelProtoType: String {
    case wireguard, openvpn, shadowsocks, none
}

struct Constants {
    static let kDefaultPathKey = "defaultPath"
    static let processQueueName = "org.amnezia.process-packets"
    static let kActivationAttemptId = "activationAttemptId"
    static let ovpnConfigKey = "ovpn"
    static let wireGuardConfigKey = "wireguard"
    static let loggerTag = "NET"

    static let kActionStart = "start"
    static let kActionRestart = "restart"
    static let kActionStop = "stop"
    static let kActionGetTunnelId = "getTunnelId"
    static let kActionStatus = "status"
    static let kActionIsServerReachable = "isServerReachable"
    static let kMessageKeyAction = "action"
    static let kMessageKeyTunnelId = "tunnelId"
    static let kMessageKeyConfig = "config"
    static let kMessageKeyErrorCode = "errorCode"
    static let kMessageKeyHost = "host"
    static let kMessageKeyPort = "port"
    static let kMessageKeyOnDemand = "is-on-demand"
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
    
    /// Internal queue.
    private let dispatchQueue = DispatchQueue(label: "PacketTunnel", qos: .utility)
    
    private var openVPNConfig: Data? = nil
      
    let vpnReachability = OpenVPNReachability()

    var startHandler: ((Error?) -> Void)?
    var stopHandler: (() -> Void)?
    var protoType: TunnelProtoType = .none
    
    override init() {
        Logger.configureGlobal(tagged: Constants.loggerTag, withFilePath: FileManager.logFileURL?.path)
        Logger.global?.log(message: "Init NEPacketTunnelProvider")
        super.init()
    }
    
    override func handleAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        Logger.global?.log(message: "Received message from app")

        guard let message = try? JSONSerialization.jsonObject(with: messageData, options: []) as? [String: Any] else {
            Logger.global?.log(message: "Failed to serialize message from app")
            return
        }

        guard let completionHandler = completionHandler else {
            Logger.global?.log(message: "Missing message completion handler")
            return
        }

        guard let action = message[Constants.kMessageKeyAction] as? String else {
            Logger.global?.log(message: "Missing action key in app message")
            completionHandler(nil)
            return
        }

        Logger.global?.log(message: "Received app message: \(action)")
        
        if action == Constants.kActionStatus {
            handleStatusAppMessage(messageData, completionHandler: completionHandler)
        }
        
        let callbackWrapper: (NSNumber?) -> Void = { errorCode in
            //let tunnelId = self.tunnelConfig?.id ?? ""
            let response: [String: Any] = [
                Constants.kMessageKeyAction: action,
                Constants.kMessageKeyErrorCode: errorCode ?? NSNull(),
                Constants.kMessageKeyTunnelId: 0
            ]

            completionHandler(try? JSONSerialization.data(withJSONObject: response, options: []))
        }

//        if action == Constants.kActionStart || action == Constants.kActionRestart {
//            Logger.global?.log(message: "Start requested")
//            self.startCompletion = callbackWrapper
//
//            if action == Constants.kActionRestart {
//                if let tunnelId = message[kMessageKeyTunnelId] as? String,
//                   let config = message[kMessageKeyConfig] {
//                    self.tunnelConfig = OutlineTunnel(id: tunnelId, config: config)
//                    self.reconnectTunnel(true)
//                }
//            }
//        } else if action == Constants.kActionStop {
//            self.stopCompletion = callbackWrapper
//        } else if action == Constants.kActionGetTunnelId {
//            var response: Data? = nil
//            if let tunnelId = self.tunnelConfig?.id {
//                response = try? JSONSerialization.data(withJSONObject: [kMessageKeyTunnelId: tunnelId], options: [])
//            }
//            completionHandler(response)
//        }
    }

    override func startTunnel(options: [String: NSObject]?, completionHandler: @escaping (Error?) -> Void) {
        dispatchQueue.async {
            let activationAttemptId = options?[Constants.kActivationAttemptId] as? String
            let errorNotifier = ErrorNotifier(activationAttemptId: activationAttemptId)
            
            Logger.global?.log(message: "PacketTunnelProvider startTunnel")
            
            if let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol {
                let providerConfiguration = protocolConfiguration.providerConfiguration
                if let _: Data = providerConfiguration?[Constants.ovpnConfigKey] as? Data {
                    self.protoType = .openvpn
                }
                else if let _: Data = providerConfiguration?[Constants.wireGuardConfigKey] as? Data {
                    self.protoType = .wireguard
                }
            }
            else {
                self.protoType = .none
            }
            
            switch self.protoType {
            case .wireguard:
                self.startWireguard(activationAttemptId: activationAttemptId,
                               errorNotifier: errorNotifier,
                               completionHandler: completionHandler)
            case .openvpn:
                self.startOpenVPN(completionHandler: completionHandler)
            case .shadowsocks:
                break
                //  startShadowSocks(completionHandler: completionHandler)
            case .none:
                break
            }
        }
    }
    
    override func stopTunnel(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        dispatchQueue.async {
            
            switch self.protoType {
            case .wireguard:
                self.stopWireguard(with: reason, completionHandler: completionHandler)
            case .openvpn:
                self.stopOpenVPN(with: reason, completionHandler: completionHandler)
            case .shadowsocks:
                break
                //  stopShadowSocks(with: reason, completionHandler: completionHandler)
            case .none:
                break
            }
        }
    }
    
    func handleStatusAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        switch protoType {
        case .wireguard:
            handleWireguardStatusMessage(messageData, completionHandler: completionHandler)
        case .openvpn:
            handleOpenVPNStatusMessage(messageData, completionHandler: completionHandler)
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
        guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
           let providerConfiguration = protocolConfiguration.providerConfiguration,
           let wgConfig: Data = providerConfiguration[Constants.wireGuardConfigKey] as? Data else {
               wg_log(.error, message: "Can't start WireGuard config missing")
               completionHandler(nil)
               return
            }
        
        let wgConfigStr = String(data: wgConfig, encoding: .utf8)!
        
        guard let tunnelConfiguration = try? TunnelConfiguration(fromWgQuickConfig: wgConfigStr) else {
            wg_log(.error, message: "Can't parse WireGuard config")
            completionHandler(nil)
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

    func handleWireguardStatusMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let completionHandler = completionHandler else { return }
        wgAdapter.getRuntimeConfiguration { settings in
            var data: Data?
            if let settings = settings {
                data = settings.data(using: .utf8)!
            }
            
            let components = settings!.components(separatedBy: "\n")
            
            var settingsDictionary: [String: String] = [:]
            for component in components{
                let pair = component.components(separatedBy: "=")
                if pair.count == 2 {
                   settingsDictionary[pair[0]] = pair[1]
                 }
            }
            
            let response: [String: Any] = [
                "rx_bytes" : settingsDictionary["rx_bytes"] ?? "0",
                "tx_bytes" : settingsDictionary["tx_bytes"] ?? "0"
            ]
            
            completionHandler(try? JSONSerialization.data(withJSONObject: response, options: []))
        }
    }
    
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

    private func handleOpenVPNStatusMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let completionHandler = completionHandler else { return }
            let bytesin = ovpnAdapter.transportStatistics.bytesIn
            let bytesout = ovpnAdapter.transportStatistics.bytesOut

            let response: [String: Any] = [
                "rx_bytes" : bytesin,
                "tx_bytes" : bytesout
            ]
            
            completionHandler(try? JSONSerialization.data(withJSONObject: response, options: []))
    }


//
    private func setupAndlaunchOpenVPN(withConfig ovpnConfiguration: Data, withShadowSocks viaSS: Bool = false, completionHandler: @escaping (Error?) -> Void) {
        wg_log(.info, message: "setupAndlaunchOpenVPN")

        let str = String(decoding: ovpnConfiguration, as: UTF8.self)

        let configuration = OpenVPNConfiguration()
        configuration.fileContent = ovpnConfiguration
        if(str.contains("cloak")){
            configuration.setPTCloak();
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

    private func startEmptyTunnel(completionHandler: @escaping (Error?) -> Void) {
        dispatchPrecondition(condition: .onQueue(dispatchQueue))

        let emptyTunnelConfiguration = TunnelConfiguration(
            name: nil,
            interface: InterfaceConfiguration(privateKey: PrivateKey()),
            peers: []
        )

        wgAdapter.start(tunnelConfiguration: emptyTunnelConfiguration) { error in
            self.dispatchQueue.async {
                if let error {
                    Logger.global?.log(message: "Failed to start an empty tunnel")
                    completionHandler(error)
                } else {
                    Logger.global?.log(message: "Started an empty tunnel")
                    self.tunnelAdapterDidStart()
                }
            }
        }

        let settings = NETunnelNetworkSettings(tunnelRemoteAddress: "1.1.1.1")

        self.setTunnelNetworkSettings(settings) { error in
            completionHandler(error)
        }
    }
    
    private func tunnelAdapterDidStart() {
        dispatchPrecondition(condition: .onQueue(dispatchQueue))
        // ...
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
