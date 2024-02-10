import Foundation

public func swiftUpdateLogData(_ qtString: std.string) -> std.string {
  let qtLog = Log(String(describing: qtString))
  var log = qtLog
  
  if let neLog = Log(at: Log.neLogURL) {
    neLog.records.forEach {
      log.records.append($0)
    }
    
    log.records.sort {
      $0.date < $1.date
    }
  }
  
  return std.string(log.description)
}

public func swiftDeleteLog() {
  Log.clear(at: Log.neLogURL)
}

public func toggleLogging(_ isEnabled: Bool) {
  Log.isLoggingEnabled = isEnabled
}
