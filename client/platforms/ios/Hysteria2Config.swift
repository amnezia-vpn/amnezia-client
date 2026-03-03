import Foundation

struct Hysteria2Config: Decodable {
    let server: String
    let port: Int
    let password: String
    let obfs: String?
    let obfsPassword: String?
    let sni: String?
    let insecure: Bool?
    let upMbps: Int?
    let downMbps: Int?
    let dns1: String?
    let dns2: String?

    enum CodingKeys: String, CodingKey {
        case server, port, password, obfs, sni, insecure
        case obfsPassword = "obfs_password"
        case upMbps = "up_mbps"
        case downMbps = "down_mbps"
        case dns1, dns2
    }
}
