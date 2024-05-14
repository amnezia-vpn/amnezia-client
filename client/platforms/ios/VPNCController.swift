import Foundation
import NetworkExtension

public func removeVPNC(_ vpncName: std.string) {
  let vpncName = String(describing: vpncName)

  Task {
    await getManagers()?.first { manager in
      if let name = manager.localizedDescription, name == vpncName {
        Task {
          await remove(manager)
        }

        return true
      } else {
        return false
      }
    }
  }
}

public func clearSettings() {
  Task {
    await getManagers()?.forEach { manager in
      Task {
        await remove(manager)
      }
    }
  }
}

func getManagers() async -> [NETunnelProviderManager]? {
  do {
    return try await NETunnelProviderManager.loadAllFromPreferences()
  } catch {
    log(.error, title: "VPNC: ", message: "loadAllFromPreferences error: \(error.localizedDescription)")
    return nil
  }
}

func remove(_ manager: NETunnelProviderManager) async {
  let vpncName = manager.localizedDescription ?? "Unknown"
  do {
    try await manager.removeFromPreferences()
    try await manager.loadFromPreferences()
    log(.info, title: "VPNC: ", message: "Remove \(vpncName)")
  } catch {
    log(.error, title: "VPNC: ", message: "Failed to remove \(vpncName) (\(error.localizedDescription))")
  }
}
