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

  private static let appGroupID = "group.org.amnezia.AmneziaVPN"

  static let appLogURL = {
    let sharedContainerURL = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: appGroupID)!
    return sharedContainerURL.appendingPathComponent("app.log", isDirectory: false)
  }()

  static let neLogURL = {
    let sharedContainerURL = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: appGroupID)!
    return sharedContainerURL.appendingPathComponent("ne.log", isDirectory: false)
  }()

  private static var sharedUserDefaults = {
    UserDefaults(suiteName: appGroupID)!
  }()

  static let dateFormatter: DateFormatter = {
    let dateFormatter = DateFormatter()
    dateFormatter.dateFormat = "yyyy-MM-dd HH:mm:ss"
    return dateFormatter
  }()

  var records = [Record]()

  var lastRecordDate = Date.distantPast

  init() {
    self.records = []
  }

  init(_ str: String) {
    records = str.split(whereSeparator: \.isNewline)
      .map {
        if let record = Record(String($0)) {
          lastRecordDate = record.date
          return record
        } else {
          return Record(date: lastRecordDate, level: .error, message: "LOG: \($0)")
        }
      }
  }

  init?(at url: URL) {
    if !FileManager.default.fileExists(atPath: url.path) {
      guard (try? "".data(using: .utf8)?.write(to: url)) != nil else { return nil }
    }

    guard let fileHandle = try? FileHandle(forUpdating: url) else { return nil }

    defer { fileHandle.closeFile() }

    guard
      let data = try? fileHandle.readToEnd(),
      let str = String(data: data, encoding: .utf8) else {
      return nil
    }

    self.init(str)
  }

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

  static func clear(at url: URL) {
    if FileManager.default.fileExists(atPath: url.path) {
      guard let fileHandle = try? FileHandle(forUpdating: url) else { return }

      defer { fileHandle.closeFile() }

      try? fileHandle.truncate(atOffset: 0)
    }
  }
}

extension Log: CustomStringConvertible {
  var description: String {
    records
      .map {
        $0.description
      }
      .joined(separator: "\n")
  }
}

func log(_ type: OSLogType, title: String = "", message: String) {
  Log.log(type, title: "App: \(title)", message: message, url: Log.appLogURL)
}
