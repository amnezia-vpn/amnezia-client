import Foundation
import os.log

public func wg_log(_ type: OSLogType, staticMessage: StaticString) {
  guard Log.isLoggingEnabled else { return }

  Log.Record(date: Date(), level: Log.Record.Level(from: type), message: "\(staticMessage)").save(at: Log.neLogURL)
}

public func wg_log(_ type: OSLogType, message: String) {
  log(type, message: message)
}

public func log(_ type: OSLogType, message: String) {
  guard Log.isLoggingEnabled else { return }

  Log.Record(date: Date(), level: Log.Record.Level(from: type), message: message).save(at: Log.neLogURL)
}
