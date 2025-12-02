import Foundation
import os.log

private let subsystemIdentifier = Bundle.main.bundleIdentifier ?? "org.amnezia.AmneziaVPN"
private let wireGuardSystemLogger = Logger(subsystem: subsystemIdentifier, category: "WireGuard")
private let openVPNSystemLogger = Logger(subsystem: subsystemIdentifier, category: "OpenVPN")
private let xraySystemLogger = Logger(subsystem: subsystemIdentifier, category: "Xray")
private let networkExtensionLogger = Logger(subsystem: subsystemIdentifier, category: "NetworkExtension")

private func logToSystem(_ logger: Logger, type: OSLogType, prefix: String, title: String, message: String) {
    let combinedTitle: String
    if title.isEmpty {
        combinedTitle = prefix
    } else {
        combinedTitle = "\(prefix): \(title)"
    }

    switch type {
    case .debug:
        if combinedTitle.isEmpty {
            logger.debug("\(message, privacy: .public)")
        } else {
            logger.debug("\(combinedTitle, privacy: .public) \(message, privacy: .public)")
        }
    case .info:
        if combinedTitle.isEmpty {
            logger.info("\(message, privacy: .public)")
        } else {
            logger.info("\(combinedTitle, privacy: .public) \(message, privacy: .public)")
        }
    case .error:
        if combinedTitle.isEmpty {
            logger.error("\(message, privacy: .public)")
        } else {
            logger.error("\(combinedTitle, privacy: .public) \(message, privacy: .public)")
        }
    case .fault:
        if combinedTitle.isEmpty {
            logger.fault("\(message, privacy: .public)")
        } else {
            logger.fault("\(combinedTitle, privacy: .public) \(message, privacy: .public)")
        }
    default:
        if combinedTitle.isEmpty {
            logger.log("\(message, privacy: .public)")
        } else {
            logger.log("\(combinedTitle, privacy: .public) \(message, privacy: .public)")
        }
    }
}

public func wg_log(_ type: OSLogType, title: String = "", staticMessage: StaticString) {
    let stringMessage = String(describing: staticMessage)
    logToSystem(wireGuardSystemLogger, type: type, prefix: "WG", title: title, message: stringMessage)
    neLog(type, title: "WG: \(title)", message: stringMessage)
}

public func wg_log(_ type: OSLogType, title: String = "", message: String) {
    logToSystem(wireGuardSystemLogger, type: type, prefix: "WG", title: title, message: message)
    neLog(type, title: "WG: \(title)", message: message)
}

public func ovpnLog(_ type: OSLogType, title: String = "", message: String) {
    logToSystem(openVPNSystemLogger, type: type, prefix: "OVPN", title: title, message: message)
    neLog(type, title: "OVPN: \(title)", message: message)
}

public func xrayLog(_ type: OSLogType, title: String = "", message: String) {
    logToSystem(xraySystemLogger, type: type, prefix: "XRAY", title: title, message: message)
    neLog(type, title: "XRAY: \(title)", message: message)
}

public func neLog(_ type: OSLogType, title: String = "", message: String) {
    logToSystem(networkExtensionLogger, type: type, prefix: "NE", title: title, message: message)
    Log.log(type, title: "NE: \(title)", message: message)
}
