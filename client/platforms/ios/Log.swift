import Foundation
import os.log

struct Log {
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

  var records: [Record]

  init() {
    self.records = []
  }

  init(_ str: String) {
    self.records = str.split(whereSeparator: \.isNewline)
      .compactMap {
        Record(String($0))!
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
