import Foundation

struct WGConfig: Decodable {
  let initPacketMagicHeader, responsePacketMagicHeader: String?
  let underloadPacketMagicHeader, transportPacketMagicHeader: String?
  let junkPacketCount, junkPacketMinSize, junkPacketMaxSize: String?
  let initPacketJunkSize, responsePacketJunkSize, cookieReplyPacketJunkSize, transportPacketJunkSize: String?
  let specialJunk1, specialJunk2, specialJunk3, specialJunk4, specialJunk5: String?
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
    func trimmed(_ value: String?) -> String? {
      guard let value = value?.trimmingCharacters(in: .whitespacesAndNewlines),
            !value.isEmpty else {
        return nil
      }
      return value
    }

    guard
      let junkPacketCount = trimmed(junkPacketCount),
      let junkPacketMinSize = trimmed(junkPacketMinSize),
      let junkPacketMaxSize = trimmed(junkPacketMaxSize),
      let initPacketJunkSize = trimmed(initPacketJunkSize),
      let responsePacketJunkSize = trimmed(responsePacketJunkSize),
      let initPacketMagicHeader = trimmed(initPacketMagicHeader),
      let responsePacketMagicHeader = trimmed(responsePacketMagicHeader),
      let underloadPacketMagicHeader = trimmed(underloadPacketMagicHeader),
      let transportPacketMagicHeader = trimmed(transportPacketMagicHeader)
    else { return "" }

    var settingsLines: [String] = []

    // Required parameters when junkPacketCount is present
    settingsLines.append("Jc = \(junkPacketCount)")
    settingsLines.append("Jmin = \(junkPacketMinSize)")
    settingsLines.append("Jmax = \(junkPacketMaxSize)")
    settingsLines.append("S1 = \(initPacketJunkSize)")
    settingsLines.append("S2 = \(responsePacketJunkSize)")

    settingsLines.append("H1 = \(initPacketMagicHeader)")
    settingsLines.append("H2 = \(responsePacketMagicHeader)")
    settingsLines.append("H3 = \(underloadPacketMagicHeader)")
    settingsLines.append("H4 = \(transportPacketMagicHeader)")

    // Optional parameters - only add if not nil and not empty
    if let s3 = trimmed(cookieReplyPacketJunkSize) {
      settingsLines.append("S3 = \(s3)")
    }
    if let s4 = trimmed(transportPacketJunkSize) {
      settingsLines.append("S4 = \(s4)")
    }

    if let i1 = trimmed(specialJunk1) {
      settingsLines.append("I1 = \(i1)")
    }
    if let i2 = trimmed(specialJunk2) {
      settingsLines.append("I2 = \(i2)")
    }
    if let i3 = trimmed(specialJunk3) {
      settingsLines.append("I3 = \(i3)")
    }
    if let i4 = trimmed(specialJunk4) {
      settingsLines.append("I4 = \(i4)")
    }
    if let i5 = trimmed(specialJunk5) {
      settingsLines.append("I5 = \(i5)")
    }

    return settingsLines.joined(separator: "\n")
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
