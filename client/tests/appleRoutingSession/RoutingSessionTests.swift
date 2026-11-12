import XCTest
import WireGuardKit
@testable import AppleRoutingIntegration

final class RoutingSessionTests: XCTestCase {
    private func configuration(full: Bool = false) -> TunnelConfiguration {
        var peer = PeerConfiguration(publicKey: PrivateKey().publicKey)
        peer.allowedIPs = (full ? ["0.0.0.0/0", "::/0"] : ["192.0.2.1/32"]).map { IPAddressRange(from: $0)! }
        if full { peer.excludeIPs = [IPAddressRange(from: "192.0.2.1/32")!] }
        return TunnelConfiguration(name: "test", interface: InterfaceConfiguration(privateKey: PrivateKey()), peers: [peer])
    }

    private func request(_ config: TunnelConfiguration, id: String = "1", session: String = "session", mode: Int = 1,
                         sites: [String] = ["192.0.2.1/32", "198.51.100.1/32"]) -> WireguardRoutingRequest {
        return WireguardRoutingRequest(action: "updateSplitTunneling", requestId: id, splitTunnelSessionId: session,
                                       publicKey: config.peers[0].publicKey.base64Key, splitTunnelType: mode,
                                       splitTunnelSites: sites)
    }

    private func started(_ config: TunnelConfiguration, mode: Int = 1) -> WireguardRoutingSession {
        let session = WireguardRoutingSession()
        session.start(id: "session", mode: mode, sites: ["192.0.2.1/32"], configuration: config)
        return session
    }

    func testInclusionAdditionPreservesImmutableConfiguration() {
        let config = configuration()
        let session = started(config)
        var applied = false
        session.update(request(config), apply: { updated, done in
            XCTAssertEqual(updated.interface, config.interface)
            XCTAssertEqual(updated.peers[0].publicKey, config.peers[0].publicKey)
            XCTAssertEqual(updated.peers[0].allowedIPs.count, 2)
            XCTAssertEqual(config.peers[0].allowedIPs.count, 1)
            done(.applied)
        }) { applied = $0 == .applied }
        XCTAssertTrue(applied)
    }

    func testExclusionAdditionPreservesDefaultAllowedIPs() {
        let config = configuration(full: true)
        let session = started(config, mode: 2)
        session.update(request(config, mode: 2), apply: { updated, done in
            XCTAssertEqual(updated.peers[0].allowedIPs, config.peers[0].allowedIPs)
            XCTAssertEqual(updated.peers[0].excludeIPs.count, 2)
            done(.applied)
        }) { XCTAssertEqual($0, .applied) }
    }

    func testInvalidAndOutdatedRequestsNeverReachAdapter() {
        let config = configuration()
        let session = started(config)
        let invalid = [request(config, session: "old"), request(config, id: ""), request(config, mode: 2),
                       request(config, sites: []), request(config, sites: ["192.0.2.1/32", "203.0.113.1/35"]),
                       request(config, sites: ["192.0.2.1/32", "::1/128"]), request(configuration()),
                       request(config, sites: ["192.0.2.1/32", "198.51.100.1/"])]
        for request in invalid {
            session.update(request, apply: { _, _ in XCTFail("Invalid request reached adapter") }) {
                XCTAssertEqual($0, .notApplied)
            }
        }
    }

    func testFullTunnelSessionDoesNotChangeOperatingSystemPolicy() {
        let config = configuration(full: true)
        let session = started(config, mode: 0)
        session.update(request(config), apply: { _, _ in XCTFail("Full tunnel must remain unchanged") }) {
            XCTAssertEqual($0, .notApplied)
        }
    }

    func testBusySessionRejectsConcurrentRequest() {
        let config = configuration()
        let session = started(config)
        var pending: ((WireGuardRoutingUpdateResult) -> Void)?
        session.update(request(config), apply: { _, done in pending = done }) { XCTAssertEqual($0, .applied) }
        session.update(request(config, id: "2"), apply: { _, _ in XCTFail("Concurrent adapter write") }) {
            XCTAssertEqual($0, .notApplied)
        }
        pending?(.applied)
    }

    func testConfirmedFailureAllowsRetry() {
        let config = configuration()
        let session = started(config)
        session.update(request(config), apply: { _, done in done(.notApplied) }) { XCTAssertEqual($0, .notApplied) }
        session.update(request(config, id: "2"), apply: { _, done in done(.applied) }) { XCTAssertEqual($0, .applied) }
    }

    func testUnknownResultBlocksFurtherRequests() {
        let config = configuration()
        let session = started(config)
        session.update(request(config), apply: { _, done in done(.stateUnknown) }) { XCTAssertEqual($0, .stateUnknown) }
        session.update(request(config, id: "2"), apply: { _, _ in XCTFail("Unknown session mutated") }) {
            XCTAssertEqual($0, .stateUnknown)
        }
    }

    func testSuccessfulAdditionCannotBeRemovedByNextRequest() {
        let config = configuration()
        let session = started(config)
        session.update(request(config), apply: { _, done in done(.applied) }) { XCTAssertEqual($0, .applied) }
        session.update(request(config, id: "2", sites: ["192.0.2.1/32"]), apply: { _, _ in XCTFail("Removal applied") }) {
            XCTAssertEqual($0, .notApplied)
        }
    }

    func testLateCallbackDoesNotUpdateNewSession() {
        let config = configuration()
        let session = started(config)
        var pending: ((WireGuardRoutingUpdateResult) -> Void)?
        session.update(request(config), apply: { _, done in pending = done }) { XCTAssertEqual($0, .stateUnknown) }
        session.stop()
        session.start(id: "new", mode: 1, sites: ["192.0.2.1/32"], configuration: config)
        pending?(.applied)
        session.update(request(config, session: "new"), apply: { _, done in done(.applied) }) { XCTAssertEqual($0, .applied) }
    }

    func testDuplicateCallbackCannotUnlockAnotherPendingRequest() {
        let config = configuration()
        let session = started(config)
        var first: ((WireGuardRoutingUpdateResult) -> Void)?
        var second: ((WireGuardRoutingUpdateResult) -> Void)?
        var completions = 0
        session.update(request(config), apply: { _, done in first = done }) { _ in completions += 1 }
        first?(.applied)
        session.update(request(config, id: "2"), apply: { _, done in second = done }) { _ in completions += 1 }
        first?(.notApplied)
        XCTAssertEqual(completions, 1)
        session.update(request(config, id: "3"), apply: { _, _ in XCTFail("Duplicate unlocked busy state") }) {
            XCTAssertEqual($0, .notApplied)
        }
        second?(.applied)
        XCTAssertEqual(completions, 2)
    }

    func testStoppedSessionDoesNotApplyUpdates() {
        let config = configuration()
        let session = started(config)
        session.stop()
        session.update(request(config), apply: { _, _ in XCTFail("Stopped session mutated") }) {
            XCTAssertEqual($0, .notApplied)
        }
    }
}
