import Foundation

struct WGConfig: Decodable {
  let initPacketMagicHeader, responsePacketMagicHeader: String?
  let underloadPacketMagicHeader, transportPacketMagicHeader: String?
  let junkPacketCount, junkPacketMinSize, junkPacketMaxSize: String?
  let initPacketJunkSize, responsePacketJunkSize: String?
  let dns1: String
  let dns2: String
  let hostName: String
  let port: Int
  let clientIP: String
  let clientPrivateKey: String
  let serverPublicKey: String
  let presharedKey: String
  var allowedIPs: [String]
  var persistentKeepAlive: String
  let splitTunnelType: Int
  let splitTunnelSites: [String]

  enum CodingKeys: String, CodingKey {
    case initPacketMagicHeader = "H1", responsePacketMagicHeader = "H2"
    case underloadPacketMagicHeader = "H3", transportPacketMagicHeader = "H4"
    case junkPacketCount = "Jc", junkPacketMinSize = "Jmin", junkPacketMaxSize = "Jmax"
    case initPacketJunkSize = "S1", responsePacketJunkSize = "S2"
    case dns1
    case dns2
    case hostName
    case port
    case clientIP = "client_ip"
    case clientPrivateKey = "client_priv_key"
    case serverPublicKey = "server_pub_key"
    case presharedKey = "psk_key"
    case allowedIPs = "allowed_ips"
    case persistentKeepAlive = "persistent_keep_alive"
    case splitTunnelType
    case splitTunnelSites
  }

  var settings: String {
    junkPacketCount == nil ? "" :
    """
    Jc = \(junkPacketCount!)
    Jmin = \(junkPacketMinSize!)
    Jmax = \(junkPacketMaxSize!)
    S1 = \(initPacketJunkSize!)
    S2 = \(responsePacketJunkSize!)
    H1 = \(initPacketMagicHeader!)
    H2 = \(responsePacketMagicHeader!)
    H3 = \(underloadPacketMagicHeader!)
    H4 = \(transportPacketMagicHeader!)

    """
  }

  var str: String {
    """
    [Interface]
    Address = \(clientIP)
    DNS = \(dns1), \(dns2)
    PrivateKey = \(clientPrivateKey)
    \(settings)
    [Peer]
    PublicKey = \(serverPublicKey)
    PresharedKey = \(presharedKey)
    AllowedIPs = \(allowedIPs.joined(separator: ", "))
    Endpoint = \(hostName):\(port)
    PersistentKeepalive = \(persistentKeepAlive)
    """
  }

  var redux: String {
    """
    [Interface]
    Address = \(clientIP)
    DNS = \(dns1), \(dns2)
    PrivateKey = ***
    \(settings)
    [Peer]
    PublicKey = ***
    PresharedKey = ***
    AllowedIPs = \(allowedIPs.joined(separator: ", "))
    Endpoint = \(hostName):\(port)
    PersistentKeepalive = \(persistentKeepAlive)
    """
  }
}

struct OpenVPNConfig: Decodable {
  let config: String
  let splitTunnelType: Int
  let splitTunnelSites: [String]

  var str: String {
    "splitTunnelType: \(splitTunnelType) splitTunnelSites: \(splitTunnelSites) config: \(config)"
  }
}
