import Foundation

struct WGConfig: Decodable {
  let h1, h2, h3, h4: String?
  let jc, jmax, jmin: String?
  let s1, s2: String?
  let dns1: String
  let dns2: String
  let mtu: String
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
    case h1 = "H1", h2 = "H2", h3 = "H3", h4 = "H4"
    case jc = "Jc", jmax = "Jmax", jmin = "Jmin"
    case s1 = "S1", s2 = "S2"
    case dns1
    case dns2
    case mtu
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
    jc == nil ? "" :
    """
    Jc = \(jc!)
    Jmin = \(jmin!)
    Jmax = \(jmax!)
    S1 = \(s1!)
    S2 = \(s2!)
    H1 = \(h1!)
    H2 = \(h2!)
    H3 = \(h3!)
    H4 = \(h4!)

    """
  }

  var str: String {
    """
    [Interface]
    Address = \(clientIP)/32
    DNS = \(dns1), \(dns2)
    MTU = \(mtu)
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
    Address = \(clientIP)/32
    DNS = \(dns1), \(dns2)
    MTU = \(mtu)
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
  let mtu: String
  let splitTunnelType: Int
  let splitTunnelSites: [String]

  var str: String {
    "splitTunnelType: \(splitTunnelType) splitTunnelSites: \(splitTunnelSites) mtu: \(mtu) config: \(config)"
  }
}
