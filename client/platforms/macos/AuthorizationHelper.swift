import Foundation
import ServiceManagement

public enum AuthorizationError: Error {
  case invalidSet
  case invalidRef
  case invalidTag
  case invalidPointer
  case denied
  case canceled
  case interactionNotAllowed
  case internalError
  case externalizeNotAllowed
  case internalizeNotAllowed
  case invalidFlags
  case toolExecuteFailure
  case toolEnvironmentError
  case badAddress
  case unknownError

  public static func checkOS(_ os: OSStatus) throws {
    switch os {
    case errAuthorizationInvalidSet: throw AuthorizationError.invalidSet
    case errAuthorizationInvalidRef: throw AuthorizationError.invalidRef
    case errAuthorizationInvalidTag: throw AuthorizationError.invalidTag
    case errAuthorizationInvalidPointer: throw AuthorizationError.invalidPointer
    case errAuthorizationDenied: throw AuthorizationError.denied
    case errAuthorizationCanceled: throw AuthorizationError.canceled
    case errAuthorizationInteractionNotAllowed: throw AuthorizationError.interactionNotAllowed
    case errAuthorizationInternal: throw AuthorizationError.internalError
    case errAuthorizationExternalizeNotAllowed: throw AuthorizationError.externalizeNotAllowed
    case errAuthorizationInternalizeNotAllowed: throw AuthorizationError.internalizeNotAllowed
    case errAuthorizationInvalidFlags: throw AuthorizationError.invalidFlags
    case errAuthorizationToolExecuteFailure: throw AuthorizationError.toolExecuteFailure
    case errAuthorizationToolEnvironmentError: throw AuthorizationError.toolEnvironmentError
    case errAuthorizationBadAddress: throw AuthorizationError.badAddress
    case errAuthorizationSuccess: break
    default: throw AuthorizationError.unknownError
    }
  }
}

private struct AuthItem: Hashable {
  public var key: String
  public var value: String

  public func withUnsafePointer<Result>(_ body: (AuthorizationItem) throws -> Result) rethrows -> Result {
    return try self.key.withCString { keyPtr in
      var valueCString = self.value.utf8CString
      var mutableValue = valueCString
      return try mutableValue.withUnsafeMutableBufferPointer { valuePtr in
        let item = AuthorizationItem(name: keyPtr, valueLength: valueCString.count, value: valueCString.count == 0 ? nil : valuePtr.baseAddress, flags: 0)

        return try body(item)
      }
    }
  }
}

extension Set where Element == AuthItem {
  func withUnsafePointer<Result>(_ body: (UnsafePointer<AuthorizationItemSet>) throws -> Result) rethrows -> Result {
    return try Set<Element>.recursiveConvert(remaining: [Element](self), converted: [], body)
  }

  static func recursiveConvert<Result>(remaining: [AuthItem], converted: [AuthorizationItem], _ body: (UnsafePointer<AuthorizationItemSet>) throws -> Result) rethrows -> Result {
    if let item = remaining.first {
      var remainingAfter = remaining
      remainingAfter.removeFirst()

      return try item.withUnsafePointer { convertedItem in
        var convertedAfter = converted
        convertedAfter.append(convertedItem)
        return try self.recursiveConvert(remaining: remainingAfter, converted: convertedAfter, body)
      }
    } else {
      // no more items to convert, calling body
      var mutableConverted = converted
      return try mutableConverted.withUnsafeMutableBufferPointer { convertedPtr in
        var itemSet = AuthorizationItemSet(count: UInt32(converted.count), items: convertedPtr.baseAddress)
        return try body(&itemSet)
      }
    }
  }
}

public struct AuthPromptEnvironment: Hashable {
  public var description: String?
  public var iconPath: String?

  init(description: String? = nil, iconPath: String? = nil) {
    self.description = description
    self.iconPath = iconPath
  }

  public func withUnsafePointer<Result>(_ body: (UnsafePointer<AuthorizationEnvironment>) throws -> Result) rethrows -> Result {
    var environmentItems: Set<AuthItem> = []
    if self.description != nil {
      environmentItems.insert(.init(key: kAuthorizationEnvironmentPrompt, value: self.description!))
    }

    if self.iconPath != nil {
      environmentItems.insert(.init(key: kAuthorizationEnvironmentIcon, value: self.iconPath!))
    }

    return try environmentItems.withUnsafePointer { itemSet in
      try body(itemSet)
    }
  }
}

public struct AuthPromptRight: Hashable {
  public var right: String

  static let blessHelper = AuthPromptRight(kSMRightBlessPrivilegedHelper)

  init(_ right: String) {
    self.right = right
  }

  fileprivate func toItem() -> AuthItem {
    .init(key: self.right, value: "")
  }
}

public class AuthorizationHelper {
  public var authRef: AuthorizationRef

  public init() throws {
    var ref: AuthorizationRef?
    try AuthorizationError.checkOS(AuthorizationCreate(nil, nil, AuthorizationFlags(), &ref))

    self.authRef = ref!
  }

  public func elevate(environment: AuthPromptEnvironment, flags: AuthorizationFlags, rights: Set<AuthPromptRight>) throws {
    try environment.withUnsafePointer { environmentPtr in
      let itemRights = Set(rights.map { $0.toItem() })
      try itemRights.withUnsafePointer { rightsPtr in
        try AuthorizationError.checkOS(AuthorizationCopyRights(self.authRef, rightsPtr, environmentPtr, flags, nil))
      }
    }
  }
}
