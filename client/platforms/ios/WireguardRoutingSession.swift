import Foundation
#if SWIFT_PACKAGE
import WireGuardKit
#endif

struct WireguardRoutingRequest: Decodable {
    let action: String
    let requestId: String
    let splitTunnelSessionId: String
    let publicKey: String
    let splitTunnelType: Int
    let splitTunnelSites: [String]

    var prefixes: [IPAddressRange]? {
        var result = [IPAddressRange]()
        for text in splitTunnelSites {
            let pieces = text.split(separator: "/", omittingEmptySubsequences: false)
            guard pieces.count == 1 || pieces.count == 2,
                  let prefix = IPAddressRange(from: text), prefix.address.rawValue.count == 4 else { return nil }
            if pieces.count == 2 {
                guard let length = UInt8(pieces[1]), length <= 32 else { return nil }
            }
            result.append(prefix)
        }
        return result
    }
}

final class WireguardRoutingSession {
    typealias Apply = (TunnelConfiguration, @escaping (WireGuardRoutingUpdateResult) -> Void) -> Void

    private let lock = NSLock()
    private var generation: UInt64 = 0
    private var sessionId: String?
    private var configuration: TunnelConfiguration?
    private var routeMode = 0
    private var sites = Set<String>()
    private var busy = false
    private var stateKnown = true

    func start(id: String?, mode: Int, sites: [String], configuration: TunnelConfiguration) {
        lock.lock()
        defer { lock.unlock() }
        generation &+= 1
        sessionId = id
        routeMode = mode
        self.sites = Set(sites)
        self.configuration = TunnelConfiguration(name: configuration.name, interface: configuration.interface,
                                                 peers: configuration.peers)
        busy = false
        stateKnown = true
    }

    func stop() {
        lock.lock()
        defer { lock.unlock() }
        generation &+= 1
        sessionId = nil
        configuration = nil
    }

    func update(_ request: WireguardRoutingRequest, apply: Apply,
                completion: @escaping (WireGuardRoutingUpdateResult) -> Void) {
        lock.lock()
        guard stateKnown else {
            lock.unlock()
            completion(.stateUnknown)
            return
        }
        guard !busy, request.action == "updateSplitTunneling", !request.requestId.isEmpty,
              !request.splitTunnelSessionId.isEmpty, request.splitTunnelSessionId == sessionId,
              (1...2).contains(routeMode), request.splitTunnelType == routeMode,
              sites.isSubset(of: Set(request.splitTunnelSites)), let prefixes = request.prefixes,
              let previous = configuration, previous.peers.count == 1,
              previous.peers[0].publicKey.base64Key == request.publicKey else {
            lock.unlock()
            completion(.notApplied)
            return
        }
        let updated = TunnelConfiguration(name: previous.name, interface: previous.interface, peers: previous.peers)
        if routeMode == 1 {
            updated.peers[0].allowedIPs = prefixes
        } else {
            updated.peers[0].excludeIPs = prefixes
        }
        let expectedGeneration = generation
        busy = true
        lock.unlock()
        var callbackPending = true
        apply(updated) { result in
            self.lock.lock()
            guard callbackPending else {
                self.lock.unlock()
                return
            }
            callbackPending = false
            guard self.generation == expectedGeneration else {
                self.lock.unlock()
                completion(.stateUnknown)
                return
            }
            self.busy = false
            if result == .applied {
                self.configuration = updated
                self.sites = Set(request.splitTunnelSites)
            } else if result == .stateUnknown {
                self.stateKnown = false
            }
            self.lock.unlock()
            completion(result)
        }
    }
}
