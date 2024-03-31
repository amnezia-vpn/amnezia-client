import Foundation
import os.log

public func wg_log(_ type: OSLogType, title: String = "", staticMessage: StaticString) {
  neLog(type, title: "WG: \(title)", message: "\(staticMessage)")
}

public func wg_log(_ type: OSLogType, title: String = "", message: String) {
  neLog(type, title: "WG: \(title)", message: message)
}

public func ovpnLog(_ type: OSLogType, title: String = "", message: String) {
  neLog(type, title: "OVPN: \(title)", message: message)
}

public func neLog(_ type: OSLogType, title: String = "", message: String) {
    Log.log(type, title: "NE: \(title)", message: message)
}
