import Foundation
import Network

@main
struct WGConfigTests {
  static func main() throws {
    let hosts = [
      ("192.0.2.10", "192.0.2.10:51820"),
      ("2001:db8::10", "[2001:db8::10]:51820"),
      ("[2001:db8::10]", "[2001:db8::10]:51820"),
      ("64:ff9b::c000:20a", "[64:ff9b::c000:20a]:51820"),
      ("vpn.example.net", "vpn.example.net:51820")
    ]
    let addresses = ["10.99.0.2/32", "10.99.0.2/32, fd99:99::2/128"]
    for (host, expectedEndpoint) in hosts {
      for address in addresses {
        let json: [String: Any] = [
          "dns1": "192.0.2.53", "dns2": "2001:db8::53", "mtu": "1280",
          "hostName": host, "port": 51820, "client_ip": address,
          "client_priv_key": "dummy", "server_pub_key": "dummy",
          "allowed_ips": ["0.0.0.0/0", "::/0"],
          "splitTunnelType": 0, "splitTunnelSites": []
        ]
        let data = try JSONSerialization.data(withJSONObject: json)
        let config = try JSONDecoder().decode(WGConfig.self, from: data)
        precondition(config.str.contains("Endpoint = \(expectedEndpoint)\n"))
        precondition(config.str.contains("Address = \(address)\n"))
        guard let endpoint = Endpoint(from: config.endpoint) else {
          fatalError("Generated endpoint rejected by WireGuardKit")
        }
        precondition(endpoint.port.rawValue == 51820)
        precondition(endpoint.stringRepresentation == expectedEndpoint)
      }
    }
    precondition(Endpoint(from: "2001:db8::10:51820") == nil)
    print("Passed 10 Apple WireGuard config cases and malformed IPv6 regression")
  }
}
