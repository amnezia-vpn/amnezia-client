import Foundation
import SecureXPC

public let VPNServiceLauncher = "com.zloserver.vpn-service-launcher"

public struct LauncherMessage: Codable {
  public let path: URL

  public init(path: URL) {
    self.path = path
  }
}

public enum LauncherError: Codable {
  case none
  case staticCodeCreate
  case validation
}

public class Routes {
  public static let launcherRoute = XPCRoute.named("launcher")
    .withMessageType(LauncherMessage.self)
    .withReplyType(LauncherError.self)
  public static let pingRoute = XPCRoute.named("ping")
    .withMessageType(LauncherPing.self)
    .withReplyType(LauncherPong.self)
}

public struct LauncherPing: Codable {
  public init() {}
}

public struct LauncherPong: Codable {
  /// This unique id is used for identifying the service, and doesn't change between pongs
  /// However if a service gets restarted, it would get changed and the client will pick up on this
  /// destroying itself.
  public let uniqueId: UUID

  public init(uniqueId: UUID) {
    self.uniqueId = uniqueId
  }
}
