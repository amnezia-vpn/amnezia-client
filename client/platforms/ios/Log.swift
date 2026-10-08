import Foundation
import os.log

struct Log {
  private static let subsystemIdentifier = Bundle.main.bundleIdentifier ?? "org.amnezia.AmneziaVPN"
  static let osLog = Logger(subsystem: subsystemIdentifier, category: "App")

  private static let IsLoggingEnabledKey = "IsLoggingEnabled"
  static var isLoggingEnabled: Bool {
    get {
      sharedUserDefaults.bool(forKey: IsLoggingEnabledKey)
    }
    set {
      sharedUserDefaults.setValue(newValue, forKey: IsLoggingEnabledKey)
    }
  }

  private static let appGroupID = BuildConfig.appGroupIdentifier

  private static let logDirectoryURL: URL = {
    if let sharedContainerURL = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: appGroupID) {
      return sharedContainerURL
    }
    return FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first
      ?? FileManager.default.temporaryDirectory
  }()

  static let appLogURL = logDirectoryURL.appendingPathComponent("app.log", isDirectory: false)

  static let neLogURL = logDirectoryURL.appendingPathComponent("ne.log", isDirectory: false)

  private static var sharedUserDefaults = {
    UserDefaults(suiteName: appGroupID) ?? .standard
  }()

  static let dateFormatter: DateFormatter = {
    let dateFormatter = DateFormatter()
    dateFormatter.locale = Locale(identifier: "en_US_POSIX")
    dateFormatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
    return dateFormatter
  }()

  static func log(_ type: OSLogType, title: String = "", message: String, url: URL = neLogURL) {
    NSLog("\(title) \(message)")

    switch type {
    case .debug:
      if title.isEmpty {
        osLog.debug("\(message, privacy: .public)")
      } else {
        osLog.debug("\(title, privacy: .public) \(message, privacy: .public)")
      }
    case .info:
      if title.isEmpty {
        osLog.info("\(message, privacy: .public)")
      } else {
        osLog.info("\(title, privacy: .public) \(message, privacy: .public)")
      }
    case .error:
      if title.isEmpty {
        osLog.error("\(message, privacy: .public)")
      } else {
        osLog.error("\(title, privacy: .public) \(message, privacy: .public)")
      }
    case .fault:
      if title.isEmpty {
        osLog.fault("\(message, privacy: .public)")
      } else {
        osLog.fault("\(title, privacy: .public) \(message, privacy: .public)")
      }
    default:
      if title.isEmpty {
        osLog.log("\(message, privacy: .public)")
      } else {
        osLog.log("\(title, privacy: .public) \(message, privacy: .public)")
      }
    }

    guard isLoggingEnabled else { return }

    let date = Date()
    let level = Record.Level(from: type)
    let messages = message.split(whereSeparator: \.isNewline)

    for index in 0..<messages.count {
      let message = String(messages[index])

      if index != 0 && message.first != " " {
        Record(date: date, level: level, message: "\(title)  \(message)").save(at: url)
      } else {
        Record(date: date, level: level, message: "\(title)\(message)").save(at: url)
      }
    }
  }
}

func log(_ type: OSLogType, title: String = "", message: String) {
  Log.log(type, title: "App: \(title)", message: message, url: Log.appLogURL)
}
