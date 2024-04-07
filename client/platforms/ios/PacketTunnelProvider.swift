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
  static let kMessageKeySplitTunnelType = "SplitTunnelType"
  static let kMessageKeySplitTunnelSites = "SplitTunnelSites"
}

class PacketTunnelProvider: NEPacketTunnelProvider {
  lazy var wgAdapter = {
    WireGuardAdapter(with: self) { logLevel, message in
      wg_log(logLevel.osLogLevel, message: message)
    }
  }()
  
  lazy var ovpnAdapter: OpenVPNAdapter = {
    let adapter = OpenVPNAdapter()
    adapter.delegate = self
    return adapter
  }()
  
  /// Internal queue.
  private let dispatchQueue = DispatchQueue(label: "PacketTunnel", qos: .utility)
  
  var splitTunnelType: Int!
  var splitTunnelSites: [String]!
  
  let vpnReachability = OpenVPNReachability()
  
  var startHandler: ((Error?) -> Void)?
  var stopHandler: (() -> Void)?
  var protoType: TunnelProtoType = .none
  
  override func handleAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
    guard let message = String(data: messageData, encoding: .utf8) else {
      if let completionHandler {
        completionHandler(nil)
      }
      return
    }
    
    neLog(.info, title: "App said: ", message: message)
    
    guard let message = try? JSONSerialization.jsonObject(with: messageData, options: []) as? [String: Any] else {
      neLog(.error, message: "Failed to serialize message from app")
      return
    }
    
    guard let completionHandler else {
      neLog(.error, message: "Missing message completion handler")
      return
    }
    
    guard let action = message[Constants.kMessageKeyAction] as? String else {
      neLog(.error, message: "Missing action key in app message")
      completionHandler(nil)
      return
    }
    
    if action == Constants.kActionStatus {
      handleStatusAppMessage(messageData, completionHandler: completionHandler)
    }
  }
  
  override func startTunnel(options: [String: NSObject]?, completionHandler: @escaping (Error?) -> Void) {
    dispatchQueue.async {
      let activationAttemptId = options?[Constants.kActivationAttemptId] as? String
      let errorNotifier = ErrorNotifier(activationAttemptId: activationAttemptId)
      
      neLog(.info, message: "Start tunnel")
      
      if let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol {
        let providerConfiguration = protocolConfiguration.providerConfiguration
        if (providerConfiguration?[Constants.ovpnConfigKey] as? Data) != nil {
          self.protoType = .openvpn
        } else if (providerConfiguration?[Constants.wireGuardConfigKey] as? Data) != nil {
          self.protoType = .wireguard
        }
      } else {
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
  
  // MARK: Network observing methods
  
  private func startListeningForNetworkChanges() {
    stopListeningForNetworkChanges()
    addObserver(self, forKeyPath: Constants.kDefaultPathKey, options: .old, context: nil)
  }
  
  private func stopListeningForNetworkChanges() {
    removeObserver(self, forKeyPath: Constants.kDefaultPathKey)
  }
  
  override func observeValue(forKeyPath keyPath: String?,
                             of object: Any?,
                             change: [NSKeyValueChangeKey: Any]?,
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
      guard let self, self.defaultPath != nil else { return }
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
