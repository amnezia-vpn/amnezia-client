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
  private lazy var wgAdapter = {
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

  // MARK: Private methods
  private func startWireguard(activationAttemptId: String?,
                              errorNotifier: ErrorNotifier,
                              completionHandler: @escaping (Error?) -> Void) {
    guard let protocolConfiguration = self.protocolConfiguration as? NETunnelProviderProtocol,
          let providerConfiguration = protocolConfiguration.providerConfiguration,
          let wgConfigData: Data = providerConfiguration[Constants.wireGuardConfigKey] as? Data else {
      wg_log(.error, message: "Can't start WireGuard config missing")
      completionHandler(nil)
      return
    }

    do {
      let wgConfig = try JSONDecoder().decode(WGConfig.self, from: wgConfigData)
      let wgConfigStr = wgConfig.str
      wg_log(.info, title: "config: ", message: wgConfig.redux)

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

      wg_log(.info, message: "Starting wireguard tunnel from the " +
             (activationAttemptId == nil ? "OS directly, rather than the app" : "app"))

      // Start the tunnel
      wgAdapter.start(tunnelConfiguration: tunnelConfiguration) { adapterError in
        guard let adapterError else {
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
      wg_log(.error, message: "Can't parse WG config: \(error.localizedDescription)")
      completionHandler(nil)
      return
    }
  }

  private func stopWireguard(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
    wg_log(.info, staticMessage: "Stopping tunnel")

    wgAdapter.stop { error in
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
      if let settings {
        data = settings.data(using: .utf8)!
      }

      let components = settings!.components(separatedBy: "\n")

      var settingsDictionary: [String: String] = [:]
      for component in components {
        let pair = component.components(separatedBy: "=")
        if pair.count == 2 {
          settingsDictionary[pair[0]] = pair[1]
        }
      }

      let response: [String: Any] = [
        "rx_bytes": settingsDictionary["rx_bytes"] ?? "0",
        "tx_bytes": settingsDictionary["tx_bytes"] ?? "0"
      ]

      completionHandler(try? JSONSerialization.data(withJSONObject: response, options: []))
    }
  }

  private func handleWireguardAppMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
    guard let completionHandler = completionHandler else { return }
    if messageData.count == 1 && messageData[0] == 0 {
      wgAdapter.getRuntimeConfiguration { settings in
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
        wgAdapter.update(tunnelConfiguration: tunnelConfiguration) { error in
          if let error {
            wg_log(.error, message: "Failed to switch tunnel configuration: \(error.localizedDescription)")
            completionHandler(nil)
            return
          }

          self.wgAdapter.getRuntimeConfiguration { settings in
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

  private func handleOpenVPNStatusMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
    guard let completionHandler = completionHandler else { return }
    let bytesin = ovpnAdapter.transportStatistics.bytesIn
    let bytesout = ovpnAdapter.transportStatistics.bytesOut

    let response: [String: Any] = [
      "rx_bytes": bytesin,
      "tx_bytes": bytesout
    ]

    completionHandler(try? JSONSerialization.data(withJSONObject: response, options: []))
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
          wg_log(.error, message: "Failed to start an empty tunnel")
          completionHandler(error)
        } else {
          wg_log(.info, message: "Started an empty tunnel")
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
