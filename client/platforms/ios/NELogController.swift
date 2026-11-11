import Foundation
import os.log

private let subsystemIdentifier = Bundle.main.bundleIdentifier ?? "org.amnezia.AmneziaVPN"
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

public func neLog(_ type: OSLogType, title: String = "", message: String) {
    logToSystem(networkExtensionLogger, type: type, prefix: "NE", title: title, message: message)
    Log.log(type, title: "NE: \(title)", message: message)
}
