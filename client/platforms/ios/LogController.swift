import Foundation

public func swiftAppLogPath() -> std.string {
  return std.string(Log.appLogURL.path)
}

public func swiftTunnelLogPath() -> std.string {
  return std.string(Log.neLogURL.path)
}

public func toggleLogging(_ isEnabled: Bool) {
  Log.isLoggingEnabled = isEnabled
}
