import Foundation
import NetworkExtension
import os
import Darwin
import OpenVPNAdapter

enum TunnelProtoType: String {
  case wireguard, openvpn, xray

}

struct Constants {
  static let kDefaultPathKey = "defaultPath"
  static let processQueueName = "org.amnezia.process-packets"
  static let kActivationAttemptId = "activationAttemptId"
  static let ovpnConfigKey = "ovpn"
  static let xrayConfigKey = "xray"
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
    var wgAdapter: WireGuardAdapter?
    var ovpnAdapter: OpenVPNAdapter?

    var splitTunnelType: Int?
    var splitTunnelSites: [String]?

    let vpnReachability = OpenVPNReachability()

    var startHandler: ((Error?) -> Void)?
    var stopHandler: (() -> Void)?
    var protoType: TunnelProtoType?

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
          handleStatusAppMessage(messageData,
                                 completionHandler: completionHandler)
      }
  }

    override func startTunnel(options: [String : NSObject]? = nil,
                              completionHandler: @escaping ((any Error)?) -> Void) {
        let activationAttemptId = options?[Constants.kActivationAttemptId] as? String
        let errorNotifier = ErrorNotifier(activationAttemptId: activationAttemptId)

        neLog(.info, message: "Start tunnel")

        if let protocolConfiguration = protocolConfiguration as? NETunnelProviderProtocol {
            let providerConfiguration = protocolConfiguration.providerConfiguration
            if (providerConfiguration?[Constants.ovpnConfigKey] as? Data) != nil {
                protoType = .openvpn
            } else if (providerConfiguration?[Constants.wireGuardConfigKey] as? Data) != nil {
                protoType = .wireguard
            } else if (providerConfiguration?[Constants.xrayConfigKey] as? Data) != nil {
                protoType = .xray
            }
        }

        guard let protoType else {
            let error = NSError(domain: "Protocol is not selected", code: 0)
            completionHandler(error)
            return
        }

        switch protoType {
        case .wireguard:
            startWireguard(activationAttemptId: activationAttemptId,
                           errorNotifier: errorNotifier,
                           completionHandler: completionHandler)
        case .openvpn:
            startOpenVPN(completionHandler: completionHandler)
        case .xray:
            startXray(completionHandler: completionHandler)

        }
    }

  
    override func stopTunnel(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
        guard let protoType else {
            completionHandler()
            return
        }

        switch protoType {
        case .wireguard:
            stopWireguard(with: reason,
                          completionHandler: completionHandler)
        case .openvpn:
            stopOpenVPN(with: reason,
                        completionHandler: completionHandler)
        case .xray:
            stopXray(completionHandler: completionHandler)
        }
    }
  
    func handleStatusAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
        guard let protoType else {
            completionHandler?(nil)
            return
        }

        switch protoType {
        case .wireguard:
            handleWireguardStatusMessage(messageData, completionHandler: completionHandler)
        case .openvpn:
            handleOpenVPNStatusMessage(messageData, completionHandler: completionHandler)
        case .xray:
            break;
        }
    }
  
    // MARK: Network observing methods
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

extension NEProviderStopReason: CustomStringConvertible {
  public var description: String {
    switch self {
    case .none:
      return "No specific reason"
    case .userInitiated:
      return "The user stopped the NE"
    case .providerFailed:
      return "The NE failed to function correctly"
    case .noNetworkAvailable:
      return "No network connectivity is currently available"
    case .unrecoverableNetworkChange:
      return "The device’s network connectivity changed"
    case .providerDisabled:
      return "The NE was disabled"
    case .authenticationCanceled:
      return "The authentication process was canceled"
    case .configurationFailed:
      return "The VPNC is invalid"
    case .idleTimeout:
      return "The session timed out"
    case .configurationDisabled:
      return "The VPNC was disabled"
    case .configurationRemoved:
      return "The VPNC was removed"
    case .superceded:
      return "VPNC was superceded by a higher-priority VPNC"
    case .userLogout:
      return "The user logged out"
    case .userSwitch:
      return "The current console user changed"
    case .connectionFailed:
      return "The connection failed"
    case .sleep:
      return "A stop reason indicating the VPNC enabled disconnect on sleep and the device went to sleep"
    case .appUpdate:
      return "appUpdat"
    @unknown default:
      return "@unknown default"
    }
  }
}
