import Foundation
import Network
import UIKit


private func section(_ out: inout String, _ name: String, _ block: () -> String) {
  out += "=== \(name) ===\n"
  out += block()
  out += "\n"
}

private func pathStatusDescription(_ status: NWPath.Status) -> String {
  switch status {
  case .satisfied: return "satisfied"
  case .unsatisfied: return "unsatisfied"
  case .requiresConnection: return "requiresConnection"
  @unknown default: return "unknown"
  }
}

private func linkTypeDescription(_ path: NWPath) -> String {
  if path.usesInterfaceType(.wifi) {
    return "wifi"
  } else if path.usesInterfaceType(.cellular) {
    return "cellular"
  } else if path.usesInterfaceType(.wiredEthernet) {
    return "ethernet"
  } else if path.usesInterfaceType(.loopback) {
    return "loopback"
  } else if path.usesInterfaceType(.other) {
    return "vpn-tunnel-or-other"
  }
  return "none"
}

private func currentPath(timeout: TimeInterval = 2.0) -> NWPath? {
  let monitor = NWPathMonitor()
  let semaphore = DispatchSemaphore(value: 0)
  var result: NWPath?
  let queue = DispatchQueue(label: "org.amnezia.network-diagnostics")

  monitor.pathUpdateHandler = { path in
    result = path
    semaphore.signal()
  }
  monitor.start(queue: queue)
  _ = semaphore.wait(timeout: .now() + timeout)
  monitor.cancel()
  return result
}

private func collectSystemInfo() -> String {
  let device = UIDevice.current
  var out = ""
  out += "iOS: \(device.systemVersion)\n"
  out += "Device: \(device.model) (\(device.name))\n"
  return out
}

private func collectLinkType(_ path: NWPath?) -> String {
  guard let path else {
    return "type=none (no path reported)\n"
  }
  var out = ""
  out += "type=\(linkTypeDescription(path))\n"
  out += "status=\(pathStatusDescription(path.status))\n"
  out += "expensive=\(path.isExpensive)\n"
  out += "constrained=\(path.isConstrained)\n"
  return out
}

private func collectAdapters() -> String {
  var out = ""
  var ifaddrPtr: UnsafeMutablePointer<ifaddrs>?
  guard getifaddrs(&ifaddrPtr) == 0, let firstAddr = ifaddrPtr else {
    out += "Failed to enumerate interfaces\n"
    return out
  }
  defer { freeifaddrs(ifaddrPtr) }

  var addressesByInterface: [String: [String]] = [:]
  var flagsByInterface: [String: UInt32] = [:]

  for ptr in sequence(first: firstAddr, next: { $0.pointee.ifa_next }) {
    let interface = ptr.pointee
    let name = String(cString: interface.ifa_name)
    flagsByInterface[name] = interface.ifa_flags

    let family = interface.ifa_addr?.pointee.sa_family
    guard family == UInt8(AF_INET) || family == UInt8(AF_INET6) else { continue }

    var hostBuffer = [CChar](repeating: 0, count: Int(NI_MAXHOST))
    let result = getnameinfo(interface.ifa_addr, socklen_t(interface.ifa_addr.pointee.sa_len),
                              &hostBuffer, socklen_t(hostBuffer.count), nil, 0, NI_NUMERICHOST)
    guard result == 0 else { continue }
    let address = String(cString: hostBuffer)
    addressesByInterface[name, default: []].append(address)
  }

  for name in flagsByInterface.keys.sorted() {
    let flags = flagsByInterface[name] ?? 0
    let isUp = (flags & UInt32(IFF_UP)) != 0
    let isLoopback = (flags & UInt32(IFF_LOOPBACK)) != 0
    out += "\(name): up=\(isUp) loopback=\(isLoopback)\n"
    for address in addressesByInterface[name] ?? [] {
      out += "  addr: \(address)\n"
    }
  }
  return out
}

private func collectRoutes(_ path: NWPath?) -> String {
  guard let path else {
    return "no active path\n"
  }
  var out = ""
  // NWPath can report the same interface twice (once per IP family); keep first occurrences.
  var seenNames = Set<String>()
  let interfaceNames = path.availableInterfaces.map { $0.name }.filter { seenNames.insert($0).inserted }
  out += "availableInterfaces=\(interfaceNames.joined(separator: ", "))\n"
  if let gatewayInterface = path.availableInterfaces.first {
    out += "primaryInterface=\(gatewayInterface.name) (\(gatewayInterface.type))\n"
  }
  out += "supportsIPv4=\(path.supportsIPv4)\n"
  out += "supportsIPv6=\(path.supportsIPv6)\n"
  out += "supportsDNS=\(path.supportsDNS)\n"
  return out
}

public func swiftRunNetworkDiagnostics(_ dnsServers: std.string) -> std.string {
  var out = ""
  let path = currentPath()

  section(&out, "system") { collectSystemInfo() }
  section(&out, "link-type") { collectLinkType(path) }
  section(&out, "adapters") { collectAdapters() }
  section(&out, "routes") { collectRoutes(path) }
  section(&out, "dns") { String(describing: dnsServers) }

  return std.string(out)
}
