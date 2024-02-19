import Foundation

struct WGConfigData: Decodable {
  let initPacketMagicHeader, responsePacketMagicHeader: String?
  let underloadPacketMagicHeader, transportPacketMagicHeader: String?
  let junkPacketCount, junkPacketMinSize, junkPacketMaxSize: String?
  let initPacketJunkSize, responsePacketJunkSize: String?

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

  let clientIP: String
  let clientPrivateKey: String
  let clientPublicKey: String
  let serverPublicKey: String
  let presharedKey: String
  let hostName: String
  let port: Int

  var allowedIPs: [String]
  var persistentKeepAlive: String

  enum CodingKeys: String, CodingKey {
    case initPacketMagicHeader = "H1", responsePacketMagicHeader = "H2"
    case underloadPacketMagicHeader = "H3", transportPacketMagicHeader = "H4"
    case junkPacketCount = "Jc", junkPacketMinSize = "Jmin", junkPacketMaxSize = "Jmax"
    case initPacketJunkSize = "S1", responsePacketJunkSize = "S2"

    case clientIP = "client_ip" // "10.8.1.16"
    case clientPrivateKey = "client_priv_key"
    case clientPublicKey = "client_pub_key"
    case serverPublicKey = "server_pub_key"
    case presharedKey = "psk_key"

    case allowedIPs = "allowed_ips"
    case persistentKeepAlive = "persistent_keep_alive"
    case hostName
    case port
  }

  init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)
    self.initPacketMagicHeader = try container.decodeIfPresent(String.self, forKey: .initPacketMagicHeader)
    self.responsePacketMagicHeader = try container.decodeIfPresent(String.self, forKey: .responsePacketMagicHeader)
    self.underloadPacketMagicHeader = try container.decodeIfPresent(String.self, forKey: .underloadPacketMagicHeader)
    self.transportPacketMagicHeader = try container.decodeIfPresent(String.self, forKey: .transportPacketMagicHeader)
    self.junkPacketCount = try container.decodeIfPresent(String.self, forKey: .junkPacketCount)
    self.junkPacketMinSize = try container.decodeIfPresent(String.self, forKey: .junkPacketMinSize)
    self.junkPacketMaxSize = try container.decodeIfPresent(String.self, forKey: .junkPacketMaxSize)
    self.initPacketJunkSize = try container.decodeIfPresent(String.self, forKey: .initPacketJunkSize)
    self.responsePacketJunkSize = try container.decodeIfPresent(String.self, forKey: .responsePacketJunkSize)
    self.clientIP = try container.decode(String.self, forKey: .clientIP)
    self.clientPrivateKey = try container.decode(String.self, forKey: .clientPrivateKey)
    self.clientPublicKey = try container.decode(String.self, forKey: .clientPublicKey)
    self.serverPublicKey = try container.decode(String.self, forKey: .serverPublicKey)
    self.presharedKey = try container.decode(String.self, forKey: .presharedKey)
    self.allowedIPs = try container.decodeIfPresent([String].self, forKey: .allowedIPs) ?? ["0.0.0.0/0", "::/0"]
    self.persistentKeepAlive = try container.decodeIfPresent(String.self, forKey: .persistentKeepAlive) ?? "25"
    self.hostName = try container.decode(String.self, forKey: .hostName)
    self.port = try container.decode(Int.self, forKey: .port)
  }
}

struct WGConfig: Decodable {
  let data: WGConfigData
  let configVersion: Int
  let description: String
  let dns1: String
  let dns2: String
  let hostName: String
  let `protocol`: String
  let splitTunnelSites: [String]
  let splitTunnelType: Int

  enum CodingKeys: String, CodingKey {
    case awgConfigData = "awg_config_data", wgConfigData = "wireguard_config_data"
    case configData
    case configVersion = "config_version"
    case description
    case dns1
    case dns2
    case hostName
    case `protocol`
    case splitTunnelSites
    case splitTunnelType
  }

  init(from decoder: Decoder) throws {
    let container = try decoder.container(keyedBy: CodingKeys.self)

    if container.contains(.awgConfigData) {
      self.data = try container.decode(WGConfigData.self, forKey: .awgConfigData)
    } else {
      self.data = try container.decode(WGConfigData.self, forKey: .wgConfigData)
    }

    self.configVersion = try container.decode(Int.self, forKey: .configVersion)
    self.description = try container.decode(String.self, forKey: .description)
    self.dns1 = try container.decode(String.self, forKey: .dns1)
    self.dns2 = try container.decode(String.self, forKey: .dns2)
    self.hostName = try container.decode(String.self, forKey: .hostName)
    self.protocol = try container.decode(String.self, forKey: .protocol)
    self.splitTunnelSites = try container.decode([String].self, forKey: .splitTunnelSites)
    self.splitTunnelType = try container.decode(Int.self, forKey: .splitTunnelType)
  }

  var str: String {
    """
    [Interface]
    Address = \(data.clientIP)/32
    DNS = \(dns1), \(dns2)
    PrivateKey = \(data.clientPrivateKey)
    \(data.settings)
    [Peer]
    PublicKey = \(data.serverPublicKey)
    PresharedKey = \(data.presharedKey)
    AllowedIPs = \(data.allowedIPs.joined(separator: ", "))
    Endpoint = \(data.hostName):\(data.port)
    PersistentKeepalive = \(data.persistentKeepAlive)
    """
  }
}
