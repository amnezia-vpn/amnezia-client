/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

import Foundation
import NetworkExtension

let vpnName = "Mozilla VPN"
var vpnBundleID = "";

@objc class VPNIPAddressRange : NSObject {
    public var address: NSString = ""
    public var networkPrefixLength: UInt8 = 0
    public var isIpv6: Bool = false

    @objc init(address: NSString, networkPrefixLength: UInt8, isIpv6: Bool) {
        super.init()

        self.address = address
        self.networkPrefixLength = networkPrefixLength
        self.isIpv6 = isIpv6
    }
}

public class IOSControllerImpl : NSObject {

    private var tunnel: NETunnelProviderManager? = nil
    private var stateChangeCallback: ((Bool) -> Void?)? = nil
    private var privateKey : PrivateKey? = nil
    private var deviceIpv4Address: String? = nil
    private var deviceIpv6Address: String? = nil

    @objc enum ConnectionState: Int { case Error, Connected, Disconnected }

    @objc init(bundleID: String, privateKey: Data, deviceIpv4Address: String, deviceIpv6Address: String, closure: @escaping (ConnectionState, Date?) -> Void, callback: @escaping (Bool) -> Void) {
        super.init()

        Logger.configureGlobal(tagged: "APP", withFilePath: "")

        vpnBundleID = bundleID;
        precondition(!vpnBundleID.isEmpty)

        stateChangeCallback = callback
        self.privateKey = PrivateKey(rawValue: privateKey)
        self.deviceIpv4Address = deviceIpv4Address
        self.deviceIpv6Address = deviceIpv6Address

        NotificationCenter.default.addObserver(self, selector: #selector(self.vpnStatusDidChange(notification:)), name: Notification.Name.NEVPNStatusDidChange, object: nil)

        NETunnelProviderManager.loadAllFromPreferences { [weak self] managers, error in
            if let error = error {
                Logger.global?.log(message: "Loading from preference failed: \(error)")
                closure(ConnectionState.Error, nil)
                return
            }

            if self == nil {
                Logger.global?.log(message: "We are shutting down.")
                return
            }

            let nsManagers = managers ?? []
            Logger.global?.log(message: "We have received \(nsManagers.count) managers.")

            let tunnel = nsManagers.first(where: IOSControllerImpl.isOurManager(_:))
            if tunnel == nil {
                Logger.global?.log(message: "Creating the tunnel")
                self!.tunnel = NETunnelProviderManager()
                closure(ConnectionState.Disconnected, nil)
                return
            }

            Logger.global?.log(message: "Tunnel already exists")

            self!.tunnel = tunnel
            if tunnel?.connection.status == .connected {
                closure(ConnectionState.Connected, tunnel?.connection.connectedDate)
            } else {
                closure(ConnectionState.Disconnected, nil)
            }
        }
    }

    @objc private func vpnStatusDidChange(notification: Notification) {
        guard let session = (notification.object as? NETunnelProviderSession), tunnel?.connection == session else { return }

        switch session.status {
        case .connected:
            Logger.global?.log(message: "STATE CHANGED: connected")
        case .connecting:
            Logger.global?.log(message: "STATE CHANGED: connecting")
        case .disconnected:
            Logger.global?.log(message: "STATE CHANGED: disconnected")
        case .disconnecting:
            Logger.global?.log(message: "STATE CHANGED: disconnecting")
        case .invalid:
            Logger.global?.log(message: "STATE CHANGED: invalid")
        case .reasserting:
            Logger.global?.log(message: "STATE CHANGED: reasserting")
        default:
            Logger.global?.log(message: "STATE CHANGED: unknown status")
        }

        // We care about "unknown" state changes.
        if (session.status != .connected && session.status != .disconnected) {
            return
        }

        stateChangeCallback?(session.status == .connected)
    }

    private static func isOurManager(_ manager: NETunnelProviderManager) -> Bool {
        guard
            let proto = manager.protocolConfiguration,
            let tunnelProto = proto as? NETunnelProviderProtocol
        else {
            Logger.global?.log(message: "Ignoring manager because the proto is invalid.")
            return false
        }

        if (tunnelProto.providerBundleIdentifier == nil) {
            Logger.global?.log(message: "Ignoring manager because the bundle identifier is null.")
            return false
        }

        if (tunnelProto.providerBundleIdentifier != vpnBundleID) {
            Logger.global?.log(message: "Ignoring manager because the bundle identifier doesn't match.")
            return false;
        }

        Logger.global?.log(message: "Found the manager with the correct bundle identifier: \(tunnelProto.providerBundleIdentifier!)")
        return true
    }

    @objc func connect(dnsServer: String, serverIpv6Gateway: String, serverPublicKey: String, serverIpv4AddrIn: String, serverPort: Int,  allowedIPAddressRanges: Array<VPNIPAddressRange>, ipv6Enabled: Bool, reason: Int, failureCallback: @escaping () -> Void) {
        Logger.global?.log(message: "Connecting")
        assert(tunnel != nil)

        // Let's remove the previous config if it exists.
        (tunnel!.protocolConfiguration as? NETunnelProviderProtocol)?.destroyConfigurationReference()

        let keyData = PublicKey(base64Key: serverPublicKey)!
        let dnsServerIP = IPv4Address(dnsServer)
        let ipv6GatewayIP = IPv6Address(serverIpv6Gateway)

        var peerConfiguration = PeerConfiguration(publicKey: keyData)
        peerConfiguration.endpoint = Endpoint(from: serverIpv4AddrIn + ":\(serverPort )")
        peerConfiguration.allowedIPs = []

        allowedIPAddressRanges.forEach {
            if (!$0.isIpv6) {
                peerConfiguration.allowedIPs.append(IPAddressRange(address: IPv4Address($0.address as String)!, networkPrefixLength: $0.networkPrefixLength))
            } else if (ipv6Enabled) {
                peerConfiguration.allowedIPs.append(IPAddressRange(address: IPv6Address($0.address as String)!, networkPrefixLength: $0.networkPrefixLength))
            }
        }

        var peerConfigurations: [PeerConfiguration] = []
        peerConfigurations.append(peerConfiguration)

        var interface = InterfaceConfiguration(privateKey: privateKey!)

        if let ipv4Address = IPAddressRange(from: deviceIpv4Address!),
           let ipv6Address = IPAddressRange(from: deviceIpv6Address!) {
            interface.addresses = [ipv4Address]
            if (ipv6Enabled) {
                interface.addresses.append(ipv6Address)
            }
        }
        interface.dns = [ DNSServer(address: dnsServerIP!)]

        if (ipv6Enabled) {
            interface.dns.append(DNSServer(address: ipv6GatewayIP!))
        }

        let config = TunnelConfiguration(name: vpnName, interface: interface, peers: peerConfigurations)

        self.configureTunnel(config: config, reason: reason, failureCallback: failureCallback)
    }

    func configureTunnel(config: TunnelConfiguration, reason: Int, failureCallback: @escaping () -> Void) {
        let proto = NETunnelProviderProtocol(tunnelConfiguration: config)
        proto!.providerBundleIdentifier = vpnBundleID

        tunnel!.protocolConfiguration = proto
        tunnel!.localizedDescription = vpnName
        tunnel!.isEnabled = true

        tunnel!.saveToPreferences { [unowned self] saveError in
            if let error = saveError {
                Logger.global?.log(message: "Connect Tunnel Save Error: \(error)")
                failureCallback()
                return
            }

            Logger.global?.log(message: "Saving the tunnel succeeded")

            self.tunnel!.loadFromPreferences { error in
                if let error = error {
                    Logger.global?.log(message: "Connect Tunnel Load Error: \(error)")
                    failureCallback()
                    return
                }

                Logger.global?.log(message: "Loading the tunnel succeeded")

                do {
                    if (reason == 1 /* ReasonSwitching */) {
                        let settings = config.asWgQuickConfig()
                        let settingsData = settings.data(using: .utf8)!
                        try (self.tunnel!.connection as? NETunnelProviderSession)?
                                .sendProviderMessage(settingsData) { data in
                            guard let data = data,
                                let configString = String(data: data, encoding: .utf8)
                            else {
                                Logger.global?.log(message: "Failed to convert response to string")
                                return
                            }
                        }
                    } else {
                        try (self.tunnel!.connection as? NETunnelProviderSession)?.startTunnel()
                    }
                } catch let error {
                    Logger.global?.log(message: "Something went wrong: \(error)")
                    failureCallback()
                    return
                }
            }
        }
    }

    @objc func disconnect() {
        Logger.global?.log(message: "Disconnecting")
        assert(tunnel != nil)
        (tunnel!.connection as? NETunnelProviderSession)?.stopTunnel()
    }

    @objc func checkStatus(callback: @escaping (String, String, String) -> Void) {
        Logger.global?.log(message: "Check status")
        assert(tunnel != nil)

        let proto = tunnel!.protocolConfiguration as? NETunnelProviderProtocol
        if proto == nil {
            callback("", "", "")
            return
        }

        let tunnelConfiguration = proto?.asTunnelConfiguration()
        if tunnelConfiguration == nil {
            callback("", "", "")
            return
        }

        let serverIpv4Gateway = tunnelConfiguration?.interface.dns[0].address
        if serverIpv4Gateway == nil {
            callback("", "", "")
            return
        }

        let deviceIpv4Address = tunnelConfiguration?.interface.addresses[0].address
        if deviceIpv4Address == nil {
            callback("", "", "")
            return
        }

        guard let session = tunnel?.connection as? NETunnelProviderSession
        else {
            callback("", "", "")
            return
        }

        do {
            try session.sendProviderMessage(Data([UInt8(0)])) { [callback] data in
                guard let data = data,
                      let configString = String(data: data, encoding: .utf8)
                else {
                    Logger.global?.log(message: "Failed to convert data to string")
                    callback("", "", "")
                    return
                }

                callback("\(serverIpv4Gateway!)", "\(deviceIpv4Address!)", configString)
            }
        } catch {
            Logger.global?.log(message: "Failed to retrieve data from session")
            callback("", "", "")
        }
    }
}
