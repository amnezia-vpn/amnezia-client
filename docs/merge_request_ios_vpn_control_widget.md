# MR Title

feat(ios): add interactive VPN control widget

# MR Description

## Summary

Adds an iOS Home Screen `systemSmall` WidgetKit widget for Amnezia VPN. The widget displays the latest known VPN state, selected protocol, and selected server/configuration name, and lets users request connect/disconnect from the Home Screen.

## Motivation

Users can start or stop the selected VPN configuration from the Home Screen without manually navigating to the in-app power button.

## Implementation Details

- Adds a `vpncontrolwidget` WidgetKit extension target to the CMake-generated Xcode project.
- Adds shared App Group state for widget display and short-lived action requests.
- Adds a small WidgetKit UI with a prominent power control, state text, protocol label, and server label.
- Adds iOS 17+ `Button(intent:)` support through `ToggleVPNWidgetIntent`.
- Adds iOS 14-16 fallback through `amneziavpn://widget/toggle`.
- Routes all actual VPN toggling through the existing `ConnectionUiController` and validation flow.
- Reloads WidgetKit timelines when connection state or selected configuration changes.

## Supported iOS Versions

- Minimum supported iOS version remains 14.0.
- Widget display is available on iOS 14+.
- WidgetKit `Button` with `AppIntent` is used on iOS 17+.
- iOS 14-16 uses the WidgetKit URL fallback.

## Behavior Differences Between iOS Versions

- iOS 14-16: tapping the widget opens the app with `amneziavpn://widget/toggle`.
- iOS 17+: tapping the power button runs an AppIntent, stores a short-lived pending toggle request in the App Group, opens the app, and the app consumes the request.

## Architecture And Shared State

The widget reads minimal state from `group.org.amnezia.AmneziaVPN`:

- connection state;
- selected protocol display name;
- selected server display name;
- saved tunnel display name;
- configuration availability;
- update and action timestamps.

The widget does not build VPN configurations and does not own connection-management logic. The main app remains the source of truth and uses the existing controllers to validate and connect/disconnect.

## Security And Privacy

- No credentials, private keys, access tokens, complete VPN configs, provider URLs, or full connection metadata are stored in widget state.
- The widget target has only the App Group entitlement.
- The widget extension does not use private APIs or Network Extension entitlements.
- Deep-link handling accepts only `amneziavpn://widget/toggle`.
- Widget action requests expire quickly and are consumed once.

## Entitlements And App Group Changes

- Adds `client/ios/widget/AmneziaVPNWidget.entitlements`.
- Requires the widget App ID and provisioning profile to include `group.org.amnezia.AmneziaVPN`.
- Existing app and packet tunnel App Group usage is preserved.

## Screenshots Or Previews

Widget previews are included for connected, disconnected, connecting, disconnecting, unavailable, and long-server-name states in `client/ios/widget/VPNControlWidgetPreviews.swift`.

## Test Plan

- Configure iOS Xcode project generation.
- Build `AmneziaVPN` Debug for a physical iPhone.
- Verify `vpncontrolwidget.appex` is embedded in the app.
- Add the widget to the Home Screen.
- Validate disconnected -> connect from the widget.
- Validate connected -> disconnect from the widget.
- Validate connecting/disconnecting state display.
- Validate missing configuration opens the app instead of attempting direct VPN control.
- Validate long server names wrap without breaking the small widget layout.
- Validate Light and Dark Mode rendering in widget previews and on device.
- Validate iOS 14-16 URL fallback on a supported device/simulator where available.
- Validate iOS 17+ AppIntent path on a supported device.

## Commands Executed

- `git diff --check` - passed.
- `plutil -lint client/ios/app/Info.plist.in client/ios/widget/Info.plist.in client/ios/widget/AmneziaVPNWidget.entitlements client/ios/widget/PrivacyInfo.xcprivacy` - passed.
- `swiftc -module-cache-path /private/tmp/amnezia-swift-module-cache -typecheck client/platforms/ios/VPNWidgetState.swift` - passed.
- `swiftc -module-cache-path /private/tmp/amnezia-swift-module-cache -parse-as-library -typecheck client/platforms/ios/VPNWidgetState.swift client/ios/widget/VPNWidgetIntent.swift` - passed.
- `swiftc -module-cache-path /private/tmp/amnezia-swift-module-cache -parse-as-library -typecheck client/platforms/ios/VPNWidgetState.swift client/ios/widget/VPNControlWidget.swift client/ios/widget/VPNWidgetIntent.swift client/ios/widget/VPNControlWidgetPreviews.swift` - failed locally because the selected Command Line Tools SDK has no iOS `UIKit` module.
- `cmake -S . -B /private/tmp/amnezia-ios-widget-configure -G Xcode -DPREBUILTS_ONLY=1 -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos` - failed locally because Xcode is not installed/selected and the iPhoneOS SDK is unavailable.
- `xcodebuild -version` - failed locally: active developer directory is Command Line Tools, not Xcode.
- `xcrun --sdk iphoneos --show-sdk-path` - failed locally: `iphoneos` SDK cannot be located.
- `cmake -S . -B /private/tmp/amnezia-host-configure -DPREBUILTS_ONLY=1` - failed locally because `conan` is not installed in `PATH`.

## Manual Testing Instructions

See `docs/ios-vpn-control-widget.md`.

## Known Limitations

- The widget opens the main app to perform VPN control. This is intentional: the widget extension does not own Network Extension entitlements or VPN configuration access.
- The widget displays the latest app-published state and does not rely on continuous WidgetKit refreshes.
- A first-time VPN permission prompt still requires the main app and iOS system confirmation.

## Rollback

Revert this MR to remove the widget target, shared widget state bridge, URL route, and CMake embedding changes. Existing VPN connection behavior is otherwise unchanged.

## Checklist

- [ ] Main iOS app builds.
- [ ] Widget extension builds.
- [ ] Packet tunnel extension remains embedded.
- [ ] App Group configured for all iOS targets.
- [ ] No credentials or signing files committed.
- [ ] Manual widget connect/disconnect verified on device.
- [ ] iOS version behavior documented.

## Related Issue

None linked.
