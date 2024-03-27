import Foundation
import os.log

struct Log {
  static let osLog = Logger()

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
