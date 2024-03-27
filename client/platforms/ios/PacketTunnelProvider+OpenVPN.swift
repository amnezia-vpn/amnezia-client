import Foundation
import NetworkExtension
import OpenVPNAdapter

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
      //      ovpnLog(.info, message: "providerConfiguration: \(String(decoding: openVPNConfigData, as: UTF8.self))")

      let openVPNConfig = try JSONDecoder().decode(OpenVPNConfig.self, from: openVPNConfigData)
      ovpnLog(.info, title: "config: ", message: openVPNConfig.str)
      let ovpnConfiguration = Data(openVPNConfig.config.utf8)
      setupAndlaunchOpenVPN(withConfig: ovpnConfiguration, completionHandler: completionHandler)
    } catch {
      ovpnLog(.error, message: "Can't parse config: \(error.localizedDescription)")

      if let underlyingError = (error as NSError).userInfo[NSUnderlyingErrorKey] as? NSError {
        ovpnLog(.error, message: "Can't parse config: \(underlyingError.localizedDescription)")
      }

      return
    }
  }

  private func setupAndlaunchOpenVPN(withConfig ovpnConfiguration: Data,
                                     withShadowSocks viaSS: Bool = false,
                                     completionHandler: @escaping (Error?) -> Void) {
    ovpnLog(.info, message: "Setup and launch")

    let str = String(decoding: ovpnConfiguration, as: UTF8.self)

    let configuration = OpenVPNConfiguration()
    configuration.fileContent = ovpnConfiguration
    if str.contains("cloak") {
      configuration.setPTCloak()
    }

    let evaluation: OpenVPNConfigurationEvaluation
    do {
      evaluation = try ovpnAdapter.apply(configuration: configuration)

    } catch {
      completionHandler(error)
      return
    }

    if !evaluation.autologin {
      ovpnLog(.info, message: "Implement login with user credentials")
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

    //        ovpn_log(.error, message: "Available TUN Interfaces: \(ifaces)")
  }

  func handleOpenVPNStatusMessage(_ messageData: Data, completionHandler: ((Data?) -> Void)? = nil) {
    guard let completionHandler = completionHandler else { return }
    let bytesin = ovpnAdapter.transportStatistics.bytesIn
    let bytesout = ovpnAdapter.transportStatistics.bytesOut

    let response: [String: Any] = [
      "rx_bytes": bytesin,
      "tx_bytes": bytesout
    ]

    completionHandler(try? JSONSerialization.data(withJSONObject: response, options: []))
  }

  func stopOpenVPN(with reason: NEProviderStopReason, completionHandler: @escaping () -> Void) {
    stopHandler = completionHandler
    if vpnReachability.isTracking {
      vpnReachability.stopTracking()
    }
    ovpnAdapter.disconnect()
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
    // Handle only fatal errors
    guard let fatal = (error as NSError).userInfo[OpenVPNAdapterErrorFatalKey] as? Bool,
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
