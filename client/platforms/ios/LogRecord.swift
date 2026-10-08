import Foundation
import os.log

extension Log {
  struct Record {
    let date: Date
    let level: Level
    let message: String

    init(date: Date, level: Level, message: String) {
      self.date = date
      self.level = level
      self.message = message
    }

    private static let saveLock = NSLock()
    private static let maxFileSize: off_t = 4 * 1024 * 1024
    private static let keepTailSize: off_t = 2 * 1024 * 1024

    func save(at url: URL) {
      osLog.log(level: level.osLogType, "\(message)")

      let data = Array("\(description)\n".utf8)

      Record.saveLock.lock()
      defer { Record.saveLock.unlock() }

      let fd = open(url.path, O_RDWR | O_APPEND | O_CREAT | O_CLOEXEC, 0o644)
      guard fd >= 0 else { return }
      defer { close(fd) }

      guard data.withUnsafeBytes({ Record.writeAll(fd, $0) }) else { return }

      var fileStat = stat()
      guard fstat(fd, &fileStat) == 0, fileStat.st_size > Record.maxFileSize else { return }

      Record.keepTail(fd, fileSize: fileStat.st_size)
    }

    private static func writeAll(_ fd: Int32, _ buffer: UnsafeRawBufferPointer) -> Bool {
      guard var pointer = buffer.baseAddress else { return true }
      var remaining = buffer.count

      while remaining > 0 {
        let written = write(fd, pointer, remaining)
        if written < 0 {
          if errno == EINTR { continue }
          return false
        }
        pointer += written
        remaining -= written
      }
      return true
    }

    private static func keepTail(_ fd: Int32, fileSize: off_t) {
      let tailSize = min(keepTailSize, fileSize)
      let tailOffset = fileSize - tailSize
      var tail = [UInt8](repeating: 0, count: Int(tailSize))
      var readTotal = 0

      while readTotal < tail.count {
        let count = tail.withUnsafeMutableBytes { buffer in
          pread(fd, buffer.baseAddress! + readTotal, buffer.count - readTotal, tailOffset + off_t(readTotal))
        }
        if count < 0 {
          if errno == EINTR { continue }
          return
        }
        if count == 0 { break }
        readTotal += count
      }

      let start = tail[0..<readTotal].firstIndex(of: UInt8(ascii: "\n")).map { $0 + 1 } ?? 0

      guard ftruncate(fd, 0) == 0 else { return }
      _ = tail[start..<readTotal].withUnsafeBytes { writeAll(fd, $0) }
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
    case error
    case fatal
    case info

    init(from osLogType: OSLogType) {
      switch osLogType {
      case .default:
        self = .info
      case .info:
        self = .info
      case .debug:
        self = .debug
      case .error:
        self = .error
      case .fault:
        self = .fatal
      default:
        self = .info
      }
    }

    var osLogType: OSLogType {
      switch self {
      case .info:
        return .info
      case .debug:
        return .debug
      case .error:
        return .error
      case .fatal:
        return .fault
      }
    }
  }
}
