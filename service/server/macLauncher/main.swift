import Foundation
import SecureXPC
import serverMacShared

func readEmbeddedPropertyList(sectionName: String) throws -> Data {
  // By passing in nil, this returns a handle for the dynamic shared object (shared library) for this helper tool
  guard let handle = dlopen(nil, RTLD_LAZY) else {
    throw XPCError.misconfiguredServer(description: "Could not read property list (handle not openable)")
  }
  defer { dlclose(handle) }

  guard let mhExecutePointer = dlsym(handle, MH_EXECUTE_SYM) else {
    throw XPCError.misconfiguredServer(description: "Could not read property list (nil symbol pointer)")
  }
  let mhExecuteBoundPointer = mhExecutePointer.assumingMemoryBound(to: mach_header_64.self)

  var size = UInt()
  guard let section = getsectiondata(mhExecuteBoundPointer, "__TEXT", sectionName, &size) else {
    throw XPCError.misconfiguredServer(description: "Missing property list section \(sectionName)")
  }

  return Data(bytes: section, count: Int(size))
}

func getSecureCodeRequiement() throws -> SecRequirement {
  do {
    let selfPlist = try readEmbeddedPropertyList(sectionName: "__info_plist")
    guard let propertyList = try PropertyListSerialization.propertyList(from: selfPlist, format: nil) as? NSDictionary else {
      NSLog("Failed to read embedded info property list")
      exit(0)
    }

    guard let authorizedExecutable = propertyList["AuthorizedExecutable"] as? String else {
      NSLog("Failed to read authorized executable from property list")
      exit(0)
    }

    var requirement: SecRequirement?
    let res = SecRequirementCreateWithString(authorizedExecutable as CFString, SecCSFlags(), &requirement)
    if res != errSecSuccess {
      NSLog("Failed to create secure requirement, reason: \(res)")
      exit(0)
    }

    return requirement!
  } catch let err {
    NSLog("Failed to read create secure code requirement \(err.localizedDescription)")
    throw err
  }
}

let codeRequirement = try! getSecureCodeRequiement()

struct ProcessData {
  public let process: Process
  public let keepalive: Pipe
  public let keepaliveTimer: DispatchSourceTimer
}

var process: ProcessData?

let server = try! XPCServer.forMachService(withCriteria: XPCServer.MachServiceCriteria.forBlessedHelperTool(withClientRequirement: XPCServer.ClientRequirement.hardenedRuntime && XPCServer.ClientRequirement.sameTeamIdentifier))

server.registerRoute(Routes.launcherRoute, handler: handleLauncher)
server.registerRoute(Routes.pingRoute, handler: handlePing)
server.setErrorHandler { error in
  NSLog("XPC error: \(error)")
}

func bootout(_ name: String) throws {
  let launchctl = Process()
  launchctl.executableURL = URL(fileURLWithPath: "/bin/launchctl")
  launchctl.arguments = ["bootout", name]
  try launchctl.run()

  launchctl.waitUntilExit()
}

let uniqueId = UUID()
func handlePing(message: LauncherPing) -> LauncherPong {
  LauncherPong(uniqueId: uniqueId)
}

func handleLauncher(message: LauncherMessage) throws -> LauncherError {
  NSLog("Handling service launching \(message.path)")
  // getting rid of old service instances
  try bootout("system/ZloVPN-service")

  let tmpFile = FileManager.default.temporaryDirectory.appending(path: "com.zloserver.vpn-service")
  try? FileManager.default.removeItem(at: tmpFile)
  try FileManager.default.copyItem(at: message.path, to: tmpFile)

  var secStaticCode: SecStaticCode?
  let res = SecStaticCodeCreateWithPath(tmpFile as CFURL, SecCSFlags(), &secStaticCode)
  if res != errSecSuccess {
    return LauncherError.staticCodeCreate
  }

  let validity = SecStaticCodeCheckValidity(secStaticCode!, SecCSFlags(), codeRequirement)
  if validity != errSecSuccess {
    return LauncherError.validation
  }

  if process != nil && process!.process.isRunning {
    process!.keepaliveTimer.cancel()
    process!.process.terminate()
  }

  let createdProcess = Process()
  createdProcess.executableURL = tmpFile
  createdProcess.currentDirectoryURL = message.path.deletingLastPathComponent()
  createdProcess.arguments = [uniqueId.uuidString]

  let pipe = Pipe()
//  createdProcess.standardInput = pipe
//  pipe.fileHandleForWriting.write(Data([0xfa]))

  let queue = DispatchQueue.global(qos: .background)
  let timer = DispatchSource.makeTimerSource(queue: queue)
  timer.setEventHandler {
    if process == nil || !process!.process.isRunning {
      NSLog("Cancelled process keepalive timer")
      timer.cancel()
      return
    }

    // process!.keepalive.fileHandleForWriting.write(Data([0xfa]))
  }
  timer.schedule(deadline: .now(), repeating: 1.0)

  process = ProcessData(process: createdProcess, keepalive: pipe, keepaliveTimer: timer)

  try process!.process.run()

  timer.resume()

  return LauncherError.none
}

server.startAndBlock()
