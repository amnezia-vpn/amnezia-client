import SwiftUI
import UIKit
import WidgetKit

struct VPNWidgetEntry: TimelineEntry {
    let date: Date
    let snapshot: VPNWidgetSnapshot
}

struct VPNWidgetProvider: TimelineProvider {
    func placeholder(in context: Context) -> VPNWidgetEntry {
        VPNWidgetEntry(date: Date(), snapshot: VPNWidgetSnapshot(
            state: .disconnected,
            protocolName: "WireGuard",
            serverName: "Sweden #1",
            tunnelName: "Sweden #1 WireGuard",
            hasConfiguration: true,
            updatedAt: Date(),
            actionLockedUntil: nil
        ))
    }

    func getSnapshot(in context: Context, completion: @escaping (VPNWidgetEntry) -> Void) {
        completion(VPNWidgetEntry(date: Date(), snapshot: VPNWidgetStore.snapshot()))
    }

    func getTimeline(in context: Context, completion: @escaping (Timeline<VPNWidgetEntry>) -> Void) {
        let entry = VPNWidgetEntry(date: Date(), snapshot: VPNWidgetStore.snapshot())
        let refreshDate = Calendar.current.date(byAdding: .minute, value: 30, to: Date()) ?? Date().addingTimeInterval(1800)
        completion(Timeline(entries: [entry], policy: .after(refreshDate)))
    }
}

struct VPNControlWidgetView: View {
    let entry: VPNWidgetEntry

    private var snapshot: VPNWidgetSnapshot {
        entry.snapshot
    }

    private var state: VPNWidgetConnectionState {
        snapshot.displayState
    }

    private var canUseIntentButton: Bool {
        snapshot.hasConfiguration && !snapshot.tunnelName.isEmpty && !state.isTransitional && !snapshot.isActionLocked
    }

    private var actionURL: URL {
        URL(string: "amneziavpn://widget/toggle")!
    }

    var body: some View {
        ZStack {
            backgroundColor

            VStack(spacing: 8) {
                Spacer(minLength: 0)

                powerControl

                VStack(spacing: 2) {
                    Text(snapshot.protocolName.isEmpty ? localized("No protocol") : snapshot.protocolName)
                        .font(.system(.caption, design: .rounded).weight(.semibold))
                        .foregroundColor(.primary)
                        .lineLimit(1)
                        .minimumScaleFactor(0.75)

                    Text(snapshot.serverName.isEmpty ? localized("No server selected") : snapshot.serverName)
                        .font(.system(.caption2, design: .rounded))
                        .foregroundColor(.secondary)
                        .lineLimit(2)
                        .multilineTextAlignment(.center)
                        .minimumScaleFactor(0.7)
                }
                .frame(maxWidth: .infinity, minHeight: 32)
            }
            .padding(12)
        }
        .widgetURL(actionURL)
    }

    @ViewBuilder
    private var powerControl: some View {
        if #available(iOSApplicationExtension 17.0, *), canUseIntentButton {
            Button(intent: ToggleVPNWidgetIntent()) {
                powerButtonContent
            }
            .buttonStyle(.plain)
        } else {
            Link(destination: actionURL) {
                powerButtonContent
            }
        }
    }

    private var powerButtonContent: some View {
        VStack(spacing: 6) {
            ZStack {
                Circle()
                    .fill(buttonFill)
                    .frame(width: 68, height: 68)
                    .shadow(color: buttonFill.opacity(0.28), radius: 10, y: 4)

                Circle()
                    .strokeBorder(buttonStroke, lineWidth: 2)
                    .frame(width: 68, height: 68)

                Image(systemName: state.isTransitional ? "arrow.triangle.2.circlepath" : "power")
                    .font(.system(size: 30, weight: .semibold))
                    .foregroundColor(.white)
                    .rotationEffect(state.isTransitional ? .degrees(18) : .zero)
            }

            Text(stateTitle)
                .font(.system(.caption2, design: .rounded).weight(.semibold))
                .foregroundColor(stateTextColor)
                .lineLimit(1)
                .minimumScaleFactor(0.7)
        }
        .accessibilityElement(children: .ignore)
        .accessibilityLabel(accessibilityLabel)
        .accessibilityValue(stateTitle)
        .accessibilityHint(accessibilityHint)
    }

    private var stateTitle: String {
        switch state {
        case .connected: return localized("Connected")
        case .disconnected: return localized("Disconnected")
        case .connecting: return localized("Connecting")
        case .disconnecting: return localized("Disconnecting")
        case .error, .unavailable: return localized("Unavailable")
        }
    }

    private var accessibilityLabel: String {
        switch state {
        case .connected: return localized("Disconnect VPN")
        case .disconnected: return localized("Connect VPN")
        default: return localized("Toggle VPN connection")
        }
    }

    private var accessibilityHint: String {
        canUseIntentButton ? localized("Double tap to connect or disconnect VPN.") : localized("Double tap to open Amnezia VPN.")
    }

    private var backgroundColor: Color {
        Color(UIColor { traits in
            traits.userInterfaceStyle == .dark
                ? UIColor(red: 0.09, green: 0.10, blue: 0.12, alpha: 1.0)
                : UIColor(red: 0.96, green: 0.97, blue: 0.98, alpha: 1.0)
        })
    }

    private var buttonFill: Color {
        switch state {
        case .connected:
            return Color(red: 0.08, green: 0.62, blue: 0.42)
        case .connecting, .disconnecting:
            return Color(red: 0.87, green: 0.53, blue: 0.18)
        case .error, .unavailable:
            return Color(red: 0.55, green: 0.58, blue: 0.62)
        case .disconnected:
            return Color(red: 0.16, green: 0.39, blue: 0.76)
        }
    }

    private var buttonStroke: Color {
        Color.white.opacity(0.48)
    }

    private var stateTextColor: Color {
        switch state {
        case .connected:
            return Color(red: 0.08, green: 0.62, blue: 0.42)
        case .connecting, .disconnecting:
            return Color(red: 0.87, green: 0.53, blue: 0.18)
        case .error, .unavailable:
            return .secondary
        case .disconnected:
            return Color(red: 0.16, green: 0.39, blue: 0.76)
        }
    }

    private func localized(_ key: String) -> String {
        NSLocalizedString(key, comment: "")
    }
}

struct AmneziaVPNControlWidget: Widget {
    let kind = "AmneziaVPNControlWidget"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: VPNWidgetProvider()) { entry in
            VPNControlWidgetView(entry: entry)
        }
        .configurationDisplayName(NSLocalizedString("Amnezia VPN", comment: ""))
        .description(NSLocalizedString("VPN", comment: ""))
        .supportedFamilies([.systemSmall])
    }
}

@main
struct AmneziaVPNWidgetBundle: WidgetBundle {
    var body: some Widget {
        AmneziaVPNControlWidget()
    }
}
