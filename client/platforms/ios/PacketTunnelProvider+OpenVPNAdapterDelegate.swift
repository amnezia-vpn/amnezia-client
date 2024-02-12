import Foundation
import NetworkExtension
import OpenVPNAdapter

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
    wg_log(.info, message: logMessage)
  }
}
