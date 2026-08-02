import Foundation

enum VPNWidgetConnectionState: String, CaseIterable {
    case connected
    case disconnected
    case connecting
    case disconnecting
    case error
    case unavailable

    var isTransitional: Bool {
        self == .connecting || self == .disconnecting
    }
}

struct VPNWidgetSnapshot {
    var state: VPNWidgetConnectionState
    var protocolName: String
    var serverName: String
    var tunnelName: String
    var hasConfiguration: Bool
    var updatedAt: Date
    var actionLockedUntil: Date?

    var isActionLocked: Bool {
        guard let actionLockedUntil else { return false }
        return actionLockedUntil > Date()
    }

    var isStale: Bool {
        Date().timeIntervalSince(updatedAt) > VPNWidgetStore.staleInterval
    }

    var displayState: VPNWidgetConnectionState {
        if isStale && state != .connected {
            return .unavailable
        }
        return state
    }
}

enum VPNWidgetStore {
    static let suiteName = "group.org.amnezia.AmneziaVPN"
    static let staleInterval: TimeInterval = 12 * 60 * 60

    private enum Key {
        static let state = "ios.widget.vpn.state"
        static let protocolName = "ios.widget.vpn.protocolName"
        static let serverName = "ios.widget.vpn.serverName"
        static let tunnelName = "ios.widget.vpn.tunnelName"
        static let hasConfiguration = "ios.widget.vpn.hasConfiguration"
        static let updatedAt = "ios.widget.vpn.updatedAt"
        static let actionLockedUntil = "ios.widget.vpn.actionLockedUntil"
        static let pendingToggleRequestedAt = "ios.widget.vpn.pendingToggleRequestedAt"
    }

    static var defaults: UserDefaults? {
        UserDefaults(suiteName: suiteName)
    }

    static func snapshot() -> VPNWidgetSnapshot {
        guard let defaults else {
            return VPNWidgetSnapshot(
                state: .unavailable,
                protocolName: "",
                serverName: "",
                tunnelName: "",
                hasConfiguration: false,
                updatedAt: .distantPast,
                actionLockedUntil: nil
            )
        }

        let state = VPNWidgetConnectionState(rawValue: defaults.string(forKey: Key.state) ?? "") ?? .unavailable
        let updatedAtSeconds = defaults.double(forKey: Key.updatedAt)
        let lockedUntilSeconds = defaults.double(forKey: Key.actionLockedUntil)

        return VPNWidgetSnapshot(
            state: state,
            protocolName: defaults.string(forKey: Key.protocolName) ?? "",
            serverName: defaults.string(forKey: Key.serverName) ?? "",
            tunnelName: defaults.string(forKey: Key.tunnelName) ?? "",
            hasConfiguration: defaults.bool(forKey: Key.hasConfiguration),
            updatedAt: updatedAtSeconds > 0 ? Date(timeIntervalSince1970: updatedAtSeconds) : .distantPast,
            actionLockedUntil: lockedUntilSeconds > 0 ? Date(timeIntervalSince1970: lockedUntilSeconds) : nil
        )
    }

    static func save(_ snapshot: VPNWidgetSnapshot) {
        guard let defaults else { return }
        defaults.set(snapshot.state.rawValue, forKey: Key.state)
        defaults.set(snapshot.protocolName, forKey: Key.protocolName)
        defaults.set(snapshot.serverName, forKey: Key.serverName)
        defaults.set(snapshot.tunnelName, forKey: Key.tunnelName)
        defaults.set(snapshot.hasConfiguration, forKey: Key.hasConfiguration)
        defaults.set(snapshot.updatedAt.timeIntervalSince1970, forKey: Key.updatedAt)
        defaults.set(snapshot.actionLockedUntil?.timeIntervalSince1970 ?? 0, forKey: Key.actionLockedUntil)
    }

    static func saveState(
        _ state: VPNWidgetConnectionState,
        protocolName: String,
        serverName: String,
        tunnelName: String,
        hasConfiguration: Bool
    ) {
        save(
            VPNWidgetSnapshot(
                state: state,
                protocolName: protocolName,
                serverName: serverName,
                tunnelName: tunnelName,
                hasConfiguration: hasConfiguration,
                updatedAt: Date(),
                actionLockedUntil: nil
            )
        )
    }

    static func setActionLock(seconds: TimeInterval) {
        var current = snapshot()
        current.actionLockedUntil = Date().addingTimeInterval(seconds)
        current.updatedAt = Date()
        save(current)
    }

    static func requestToggleFromApp() {
        guard let defaults else { return }
        let now = Date()
        defaults.set(now.timeIntervalSince1970, forKey: Key.pendingToggleRequestedAt)
        setActionLock(seconds: 12)
    }

    static func consumeToggleRequest(maxAge: TimeInterval = 30) -> Bool {
        guard let defaults else { return false }
        let requestedAt = defaults.double(forKey: Key.pendingToggleRequestedAt)
        guard requestedAt > 0 else { return false }

        defaults.set(0, forKey: Key.pendingToggleRequestedAt)
        return Date().timeIntervalSince1970 - requestedAt <= maxAge
    }
}
