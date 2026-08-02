import AppIntents
import Foundation
import WidgetKit

@available(iOS 16.0, *)
struct ToggleVPNWidgetIntent: AppIntent {
    static var title: LocalizedStringResource = "Toggle VPN connection"
    static var openAppWhenRun: Bool = true

    func perform() async throws -> some IntentResult {
        let snapshot = VPNWidgetStore.snapshot()

        guard snapshot.hasConfiguration,
              !snapshot.tunnelName.isEmpty,
              !snapshot.displayState.isTransitional,
              !snapshot.isActionLocked else {
            return .result()
        }

        VPNWidgetStore.requestToggleFromApp()
        WidgetCenter.shared.reloadTimelines(ofKind: "AmneziaVPNControlWidget")
        return .result()
    }
}
