import Foundation
import WidgetKit

public func swiftUpdateVPNWidgetState(
    _ stateValue: std.string,
    _ protocolValue: std.string,
    _ serverValue: std.string,
    _ tunnelValue: std.string,
    _ hasConfiguration: Bool
) {
    let state = VPNWidgetConnectionState(rawValue: String(describing: stateValue)) ?? .unavailable
    VPNWidgetStore.saveState(
        state,
        protocolName: String(describing: protocolValue),
        serverName: String(describing: serverValue),
        tunnelName: String(describing: tunnelValue),
        hasConfiguration: hasConfiguration
    )

    WidgetCenter.shared.reloadTimelines(ofKind: "AmneziaVPNControlWidget")
}

public func swiftConsumeVPNWidgetToggleRequest() -> Bool {
    VPNWidgetStore.consumeToggleRequest()
}
