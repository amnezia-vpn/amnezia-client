import Foundation

@objcMembers
public final class FBLinkIOSBridge: NSObject {
  public static func swiftUpdateLogData(_ qtString: String) -> String {
    let qtLog = Log(qtString)
    var log = qtLog

    if let appLog = Log(at: Log.appLogURL) {
      appLog.records.forEach {
        log.records.append($0)
      }
    }

    if let neLog = Log(at: Log.neLogURL) {
      neLog.records.forEach {
        log.records.append($0)
      }
    }

    log.records.sort {
      $0.date < $1.date
    }

    return log.description
  }

  public static func swiftDeleteLog() {
    Log.clear(at: Log.appLogURL)
    Log.clear(at: Log.neLogURL)
  }

  public static func toggleLogging(_ isEnabled: Bool) {
    Log.isLoggingEnabled = isEnabled
  }
}
