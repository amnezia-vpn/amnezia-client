# iOS VPN Control Widget

Amnezia VPN includes a WidgetKit Home Screen widget for the iOS `systemSmall` family. The widget shows the latest known VPN state, selected protocol, and selected server/configuration display name.

## Behavior

- iOS 14-16: tapping the widget opens `amneziavpn://widget/toggle`; the main app consumes the URL and uses the existing connection flow.
- iOS 17 and later: the widget uses a WidgetKit `Button` backed by an `AppIntent`. The intent records a short-lived toggle request in the App Group and opens the main app, where the existing Qt connection controller validates and toggles the VPN.
- The widget extension does not store VPN credentials, private keys, complete configurations, access tokens, or server connection metadata.
- The widget extension does not require Network Extension entitlements and does not attempt to control `NETunnelProviderManager` directly.

## Shared State

The app writes minimal display state to `group.org.amnezia.AmneziaVPN`:

- connection state;
- selected protocol display name;
- selected server display name;
- saved tunnel manager display name;
- configuration availability flag;
- update timestamp;
- short action lock and pending action timestamp.

The main app reloads the widget timeline when VPN state, selected server, or selected protocol changes.

## Signing And App Groups

The generated Xcode project contains three iOS targets:

- `AmneziaVPN`;
- `networkextension`;
- `vpncontrolwidget`.

Apple Developer configuration must include:

- App ID for `org.amnezia.AmneziaVPN`;
- App ID for `org.amnezia.AmneziaVPN.network-extension`;
- App ID for `org.amnezia.AmneziaVPN.widget`;
- App Group `group.org.amnezia.AmneziaVPN` enabled for the app, packet tunnel extension, and widget extension;
- provisioning profiles for all three targets.

When building with custom bundle identifiers, keep the App Group value consistent with `BUILD_IOS_GROUP_IDENTIFIER` and the entitlements files.

## Local Build

Generate the Xcode project through the existing CMake/Qt process, then open the generated `AmneziaVPN.xcodeproj` from the build directory.

Example:

```bash
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos
cmake --build build-ios --config Debug --target AmneziaVPN
```

For the repository release flow:

```bash
sh deploy/build.sh -t ios
```

## Physical Device Validation

1. Open the generated `AmneziaVPN.xcodeproj`.
2. Select the `AmneziaVPN` scheme.
3. Select a physical iPhone running iOS 14 or later.
4. Configure signing for `AmneziaVPN`, `networkextension`, and `vpncontrolwidget`.
5. Verify all three targets use the same App Group.
6. Build and run the app.
7. Import or select a VPN configuration in the app.
8. Connect once from the app so iOS creates and authorizes the Network Extension tunnel.
9. Long press the Home Screen, tap `+`, select Amnezia VPN, and add the small widget.
10. Tap the widget power button.
11. Verify the app opens and the existing connection flow connects or disconnects the selected configuration.
12. Verify the widget updates after state changes.

## Logs

If the action fails:

- collect the app log from the existing Amnezia diagnostics flow;
- in Xcode, filter the device console for `AmneziaVPN`, `vpncontrolwidget`, and `WidgetKit`;
- confirm the widget target has the App Group entitlement;
- confirm the main app receives `amneziavpn://widget/toggle` on iOS 14-16 or consumes the pending AppIntent action on iOS 17+.

Do not attach VPN credentials, private keys, full provider configurations, or provisioning secrets to issue reports.
