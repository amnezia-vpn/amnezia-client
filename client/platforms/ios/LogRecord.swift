import Foundation
import os.log

extension Log {
  struct Record {
    let date: Date
    let level: Level
    let message: String

    init?(_ str: String) {
      let dateStr = String(str.prefix(19))
      guard let date = Log.dateFormatter.date(from: dateStr) else { return nil }

      let str = str.dropFirst(20)

      guard let endIndex = str.firstIndex(of: " ") else { return nil }
      let levelStr = String(str[str.startIndex..<endIndex])
      guard let level = Level(rawValue: levelStr) else { return nil }

      let messageStartIndex = str.index(after: endIndex)
      let message = String(str[messageStartIndex..<str.endIndex])

      self.init(date: date, level: level, message: message)
    }

    init(date: Date, level: Level, message: String) {
      self.date = date
      self.level = level
      self.message = message
    }

    func save(at url: URL) {
      guard let data = "\n\(description)".data(using: .utf8) else { return }

      if !FileManager.default.fileExists(atPath: url.path) {
        guard (try? "".data(using: .utf8)?.write(to: url)) != nil else { return }
      }

      guard let fileHandle = try? FileHandle(forUpdating: url) else { return }

      defer { fileHandle.closeFile() }

      guard (try? fileHandle.seekToEnd()) != nil else { return }
      try? fileHandle.write(contentsOf: data)
    }
  }
}

extension Log.Record: CustomStringConvertible {
  var description: String {
    "\(Log.dateFormatter.string(from: date)) \(level.rawValue) \(message)"
  }
}

extension Log.Record {
  enum Level: String {
    case debug
    case warning
    case error
    case critical
    case fatal
    case info
    case system // critical

    init(from osLogType: OSLogType) {
      switch osLogType {
      case OSLogType.default:
        self = .info
      case OSLogType.info:
        self = .info
      case OSLogType.debug:
        self = .debug
      case OSLogType.error:
        self = .error
      case OSLogType.fault:
        self = .fatal
      default:
        self = .info
      }
    }
  }
}
