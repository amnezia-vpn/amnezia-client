import SwiftUI
import WidgetKit

#if DEBUG
struct VPNControlWidgetPreviews: PreviewProvider {
    static var previews: some View {
        Group {
            preview(.connected, protocolName: "WireGuard", serverName: "Sweden #1")
                .previewDisplayName("Connected")
            preview(.disconnected, protocolName: "AmneziaWG", serverName: "Netherlands")
                .previewDisplayName("Disconnected")
            preview(.connecting, protocolName: "OpenVPN", serverName: "Germany #2")
                .previewDisplayName("Connecting")
            preview(.disconnecting, protocolName: "XRay", serverName: "Finland")
                .previewDisplayName("Disconnecting")
            preview(.unavailable, protocolName: "", serverName: "")
                .previewDisplayName("Unavailable")
            preview(.disconnected, protocolName: "WireGuard", serverName: "A very long selected server display name")
                .previewDisplayName("Long Server")
        }
        .previewContext(WidgetPreviewContext(family: .systemSmall))
    }

    private static func preview(
        _ state: VPNWidgetConnectionState,
        protocolName: String,
        serverName: String
    ) -> VPNControlWidgetView {
        VPNControlWidgetView(entry: VPNWidgetEntry(
            date: Date(),
            snapshot: VPNWidgetSnapshot(
                state: state,
                protocolName: protocolName,
                serverName: serverName,
                tunnelName: serverName.isEmpty ? protocolName : "\(serverName) \(protocolName)",
                hasConfiguration: !protocolName.isEmpty && !serverName.isEmpty,
                updatedAt: Date(),
                actionLockedUntil: nil
            )
        ))
    }
}
#endif
