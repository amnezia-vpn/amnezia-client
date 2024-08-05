import Foundation

struct XrayConfig: Decodable {
    let dns1: String?
    let dns2: String?
    let config: String
}
