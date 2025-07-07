import Foundation

struct WGConfig: Decodable {
  let initPacketMagicHeader, responsePacketMagicHeader: String?
  let underloadPacketMagicHeader, transportPacketMagicHeader: String?
  let junkPacketCount, junkPacketMinSize, junkPacketMaxSize: String?
  let initPacketJunkSize, responsePacketJunkSize, cookieReplyPacketJunkSize, transportPacketJunkSize: String?
  let specialJunk1, specialJunk2, specialJunk3, specialJunk4, specialJunk5: String?
  let controlledJunk1, controlledJunk2, controlledJunk3: String?
  let specialHandshakeTimeout: String?
  let dns1: String
  let dns2: String
  let mtu: String
  let hostName: String
  let port: Int
  let clientIP: String
  let clientPrivateKey: String
  let serverPublicKey: String
  let presharedKey: String?
  var allowedIPs: [String]
  var persistentKeepAlive: String
  let splitTunnelType: Int
  let splitTunnelSites: [String]

  enum CodingKeys: String, CodingKey {
    case initPacketMagicHeader = "H1", responsePacketMagicHeader = "H2"
    case underloadPacketMagicHeader = "H3", transportPacketMagicHeader = "H4"
    case junkPacketCount = "Jc", junkPacketMinSize = "Jmin", junkPacketMaxSize = "Jmax"
    case initPacketJunkSize = "S1", responsePacketJunkSize = "S2", cookieReplyPacketJunkSize = "S3", transportPacketJunkSize = "S4"
    case specialJunk1 = "I1", specialJunk2 = "I2", specialJunk3 = "I3", specialJunk4 = "I4", specialJunk5 = "I5"
    case controlledJunk1 = "J1", controlledJunk2 = "J2", controlledJunk3 = "J3"
    case specialHandshakeTimeout = "Itime"
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
    junkPacketCount == nil ? "" :
    """
    Jc = \(junkPacketCount!)
    Jmin = \(junkPacketMinSize!)
    Jmax = \(junkPacketMaxSize!)
    S1 = \(initPacketJunkSize!)
    S2 = \(responsePacketJunkSize!)
    S3 = \(cookieReplyPacketJunkSize!)
    S4 = \(transportPacketJunkSize!)
    H1 = \(initPacketMagicHeader!)
    H2 = \(responsePacketMagicHeader!)
    H3 = \(underloadPacketMagicHeader!)
    H4 = \(transportPacketMagicHeader!)
    I1 = \(specialJunk1!)
    I2 = \(specialJunk2!)
    I3 = \(specialJunk3!)
    I4 = \(specialJunk4!)
    I5 = \(specialJunk5!)
    J1 = \(controlledJunk1!)
    J2 = \(controlledJunk2!)
    J3 = \(controlledJunk3!)
    Itime = \(specialHandshakeTimeout!)
    """
  }

  var str: String {
    """
    [Interface]
    Address = \(clientIP)
    DNS = \(dns1), \(dns2)
    MTU = \(mtu)
    PrivateKey = \(clientPrivateKey)
    \(settings)
    [Peer]
    PublicKey = \(serverPublicKey)
    \(presharedKey == nil ? "" : "PresharedKey = \(presharedKey!)")
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
    MTU = \(mtu)
    PrivateKey = ***
    \(settings)
    [Peer]
    PublicKey = ***
    PresharedKey = ***
    AllowedIPs = \(allowedIPs.joined(separator: ", "))
    Endpoint = \(hostName):\(port)
    PersistentKeepalive = \(persistentKeepAlive)

    SplitTunnelType = \(splitTunnelType)
    SplitTunnelSites = \(splitTunnelSites.joined(separator: ", "))
    """
  }
}
