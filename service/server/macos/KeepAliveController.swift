import CxxStdlib
import SecureXPC
import serverMacShared
import ServiceManagement

import ZloVPNServiceCpp

var client: XPCClient?
var uniqueId: UUID?
var errorCount = 0

func keepaliveTimeout() {}

public func initializeKeepalive(stdStringToken: std.string) -> Bool {
  let token = String(stdStringToken)
  uniqueId = UUID(uuidString: token)

  logDebug(std.string("[Swift] Initializing keepalive"))

  if uniqueId == nil {
    logError(std.string("Failed to initialize keepalive, invalid id: \(token)"))
    return false
  }

  client = XPCClient.forMachService(named: VPNServiceLauncher, withServerRequirement: try! XPCClient.ServerRequirement.sameTeamIdentifier)

  let queue = DispatchQueue.global(qos: .background)
  let timer = DispatchSource.makeTimerSource(queue: queue)
  timer.setEventHandler {
    Task {
      if uniqueId == nil {
        timer.cancel()
        return
      }

      do {
        let response = try await client!.sendMessage(LauncherPing(), to: Routes.pingRoute)
        if response.uniqueId != uniqueId! {
          logError(std.string("Keepalive uniqueId mismatch, exiting..."))
          timer.cancel()
          keepaliveTimeout()
        }
      } catch let err {
        logError(std.string("Failed to receive keepalive: \(err.localizedDescription)"))
        errorCount += 1
        if errorCount == 5 {
          logError(std.string("Request error limit exceeded, exiting..."))
          timer.cancel()
          keepaliveTimeout()
        }
      }
    }
  }
  timer.schedule(deadline: .now(), repeating: 1.0)
  timer.resume()

  return true
}
