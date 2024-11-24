import CxxStdlib
import SecureXPC
import serverMacShared
import ServiceManagement

// TODO: replace with SMAppService once apple fixes the bundle renaming bug

let VPNService = "com.zloserver.vpn-service"

public func firstSetupNeeded() -> Bool {
  let plistUrl = URL(fileURLWithPath: "/Library/LaunchDaemons/\(VPNServiceLauncher).plist")
  let status = SMAppService.statusForLegacyPlist(at: plistUrl)
  if status == .enabled {
    guard let data = try? Data(contentsOf: plistUrl) else {
      return true
    }

    guard let propertyList = try? PropertyListSerialization.propertyList(from: data, options: PropertyListSerialization.ReadOptions(), format: nil) as? NSDictionary else {
      return true
    }

    guard let version = propertyList["Version"] as? String else {
      return true
    }

    guard let selfVersion = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String else {
      return true
    }

    return selfVersion != version
  }
  return status != .enabled
}

public enum FirstSetupError {
  case None
  case MissingDaemonPlist
}

public struct FirstSetupResponse {
  public var authorizationError: AuthorizationError?
  public var blessError: std.string?
  public var otherError: std.string?

  public init() {}

  public init(authorizationError: AuthorizationError) {
    self.authorizationError = authorizationError
  }

  public init(blessError: String) {
    self.blessError = std.string(blessError)
  }

  public init(otherError: String) {
    self.otherError = std.string(otherError)
  }

  public func isError() -> Bool {
    self.authorizationError != nil || self.blessError != nil || self.otherError != nil
  }

  public func getErrorString() -> std.string {
    if self.authorizationError != nil {
      return std.string(self.authorizationError!.localizedDescription)
    } else if self.blessError != nil {
      return self.blessError!
    } else if self.otherError != nil {
      return self.otherError!
    }

    return std.string("")
  }

  public func requiresApproval() -> Bool {
    self.authorizationError != nil || self.blessError != nil
  }
}

public func doFirstSetup(message: std.string) -> FirstSetupResponse {
  do {
    let authHelper = try AuthorizationHelper()

    let env = AuthPromptEnvironment(description: String(message))
    let flags = AuthorizationFlags(rawValue: AuthorizationFlags.interactionAllowed.rawValue | AuthorizationFlags.preAuthorize.rawValue | AuthorizationFlags.extendRights.rawValue)
    let rights = [AuthPromptRight.blessHelper]
    try authHelper.elevate(environment: env, flags: flags, rights: Set(rights))

    var cfError: Unmanaged<CFError>? = nil
    let bless = SMJobBless(kSMDomainSystemLaunchd, VPNServiceLauncher as CFString, authHelper.authRef, &cfError)

    if cfError != nil {
      let err = cfError!.takeRetainedValue()
      return FirstSetupResponse(blessError: err.localizedDescription)
    }

    restartService()

    return FirstSetupResponse()
  } catch let err as AuthorizationError {
    return FirstSetupResponse(authorizationError: err)
  } catch let err {
    return FirstSetupResponse(otherError: err.localizedDescription)
  }
}

public enum LauncherErrorCxx: UInt32 {
  case none
  case staticCodeCreate
  case validation

  init(_ launcherError: LauncherError) {
    switch launcherError {
    case LauncherError.none: self = .none
    case LauncherError.staticCodeCreate: self = .staticCodeCreate
    case LauncherError.validation: self = .validation
    }
  }
}

public struct RestartServiceResult {
  public let errorMessage: std.string?
  public let launcherResponse: LauncherErrorCxx?
}

public func restartService() -> RestartServiceResult {
  let client = XPCClient.forMachService(named: VPNServiceLauncher, withServerRequirement: try! XPCClient.ServerRequirement.sameTeamIdentifier)

  let executablePath = Bundle.main.bundleURL.appending(path: "Contents/MacOS/\(VPNService)")

  let semaphore = DispatchSemaphore(value: 0)
  var res: Result<LauncherError, Error>?
  Task {
    do {
      res = try Result.success(await client.sendMessage(LauncherMessage(path: executablePath), to: Routes.launcherRoute))
    } catch let err {
      res = Result.failure(err)
    }
    semaphore.signal()
  }
  semaphore.wait()

  switch res {
  case .failure(let err):
    return RestartServiceResult(errorMessage: std.string(err.localizedDescription), launcherResponse: nil)
  case .success(let launcherResponse):
    if case .none = launcherResponse {
      return RestartServiceResult(errorMessage: nil, launcherResponse: nil)
    } else {
      return RestartServiceResult(errorMessage: nil, launcherResponse: LauncherErrorCxx(launcherResponse))
    }
  default:
    return RestartServiceResult(errorMessage: "Unknown failure", launcherResponse: nil)
  }
}
