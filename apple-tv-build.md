# AmneziaVPN Apple TV Build

This document describes how to build the current branch for Apple TV from the repository root.

The pipeline is:

1. Use a separately built static Qt 6.9.2 for `tvOS`.
2. Let Conan build/provide C/C++ dependencies.
3. Generate an Xcode project with `qt-cmake`.
4. Build the `.app` and embedded Network Extension with `xcodebuild`.

Important:

- Run the project commands from the repository root.
- This is a device build for `appletvos`, not a simulator build.
- `xcodebuild build` produces `.app`.
- `.ipa` is produced later via `archive` and `-exportArchive`.
- The current tvOS Network Extension scope is WireGuard-only.
- The temporary tvOS WireGuard bridge prebuilt is opt-in. The Conan recipe does not contain machine-specific fallback paths.
- Do not initialize or update submodules just for this build. If a clean checkout has empty `client/3rd` folders, pass `AMNEZIA_THIRDPARTY_ROOT` to an already initialized read-only `client/3rd` tree.

## 1. Environment

Set these paths for your machine:

```bash
export REPO_ROOT="$PWD"
export QT_DESKTOP_PREFIX="$HOME/Qt/6.9.2/macos"
export QT_TVOS_SRC="$HOME/Qt_tv/qt-6.9.2-tvos-src"
export QT_TVOS_PREFIX="$HOME/Qt_tv/6.9.2/tvos-device"
export BUILD_DIR="$REPO_ROOT/build-tvos-device-conan"
```

If this checkout does not have initialized `client/3rd` sources, point CMake at an initialized tree:

```bash
export AMNEZIA_THIRDPARTY_ROOT="/path/to/initialized/amnezia/client/3rd"
```

If you are using a temporary prebuilt tvOS WireGuard bridge, point Conan at it explicitly:

```bash
export AMNEZIA_TVOS_AWG_PREBUILT_DIR="/path/to/WireGuardKitGo-appletvos"
export AMNEZIA_TVOS_AWG_VERSION_HEADER_DIR="/path/to/directory/with/wireguard-go-version.h"
```

`AMNEZIA_TVOS_AWG_PREBUILT_DIR` must contain `libwg-go.a`.

`AMNEZIA_TVOS_AWG_VERSION_HEADER_DIR` is optional when `wireguard-go-version.h` lives in the same directory as `libwg-go.a`.

If the env vars are not set, the recipe uses the normal source build path. Rebuilding and publishing the tvOS WireGuard bridge through the registry is a separate task.

## 2. Required Local Tools

Conan must be available:

```bash
uv tool install conan
export PATH="$HOME/.local/bin:$PATH"
conan --version
```

Validated version:

```text
Conan version 2.27.1
```

The build uses Xcode's AppleTVOS SDK:

```bash
xcrun --sdk appletvos --show-sdk-path
```

## 3. Prepare Qt Sources

Do not edit the installed Qt sources in place. Copy them into a separate tvOS fork:

```bash
mkdir -p "$HOME/Qt_tv"
rsync -a "$HOME/Qt/6.9.2/Src/" "$QT_TVOS_SRC/"
```

Recommended for reproducibility:

```bash
cd "$QT_TVOS_SRC"
git init
git add .
git commit -m "Qt 6.9.2 source snapshot"
```

## 4. Apply the Qt tvOS Patchset

Apply the local Qt tvOS patchset to `$QT_TVOS_SRC`.

If you need to recreate the patchset from a fresh copy, compare these files against `$HOME/Qt/6.9.2/Src` and reapply the same changes:

- `qtbase/cmake/QtBaseGlobalTargets.cmake`
- `qtbase/cmake/QtBaseHelpers.cmake`
- `qtbase/cmake/QtBuildPathsHelpers.cmake`
- `qtbase/cmake/QtMkspecHelpers.cmake`
- `qtbase/cmake/QtConfig.cmake.in`
- `qtbase/mkspecs/unsupported/macx-tvos-clang/qplatformdefs.h`
- `qtbase/src/corelib/CMakeLists.txt`
- `qtbase/src/corelib/platform/darwin/qdarwinpermissionplugin_location.mm`
- `qtbase/src/gui/CMakeLists.txt`
- `qtbase/src/widgets/CMakeLists.txt`
- `qtbase/src/network/kernel/qnetworkproxy_darwin.cpp`
- `qtbase/src/testlib/qtestcrashhandler.cpp`
- `qtbase/src/plugins/platforms/ios/qiosapplicationdelegate.mm`
- `qtbase/src/plugins/platforms/ios/qiosscreen.mm`
- `qtbase/src/plugins/platforms/ios/qiostheme.mm`
- `qtbase/src/plugins/platforms/ios/quiview.mm`
- `qtbase/src/plugins/platforms/ios/qiosclipboard.mm`

Recommended after patching:

```bash
git -C "$QT_TVOS_SRC" diff > "$HOME/Qt_tv/qt-6.9.2-tvos.patch"
```

Do not use `QT_APPLE_SDK=appletvos`. The working path is `CMAKE_SYSTEM_NAME=tvOS` with `CMAKE_OSX_SYSROOT=appletvos`.

## 5. Build Qt 6.9.2 for Apple TV

Only the modules required by this project are built.

```bash
mkdir -p /private/tmp/qt6.9.2-tvos-device-build
cd /private/tmp/qt6.9.2-tvos-device-build

"$QT_TVOS_SRC/configure" \
  -release -static -appstore-compliant \
  -nomake tests -nomake examples \
  -submodules qtbase,qtdeclarative,qtshadertools,qtremoteobjects,qtsvg,qt5compat,qttools \
  -qt-host-path "$QT_DESKTOP_PREFIX" \
  -prefix "$QT_TVOS_PREFIX" \
  -- \
  -G Ninja \
  -DQT_QMAKE_TARGET_MKSPEC=macx-tvos-clang \
  -DCMAKE_SYSTEM_NAME=tvOS \
  -DCMAKE_OSX_SYSROOT=appletvos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 \
  -DBUILD_SHARED_LIBS=OFF \
  -DQT_NO_APPLE_SDK_MAX_VERSION_CHECK=ON

cmake --build . --parallel 8
cmake --install .
```

Sanity checks:

```bash
"$QT_TVOS_PREFIX/bin/qt-cmake" --version
"$QT_TVOS_PREFIX/bin/qmake" -query QMAKE_XSPEC
```

Expected `QMAKE_XSPEC`:

```text
macx-tvos-clang
```

Return to the repository root after building Qt:

```bash
cd "$REPO_ROOT"
```

## 6. Conan Dependency Behavior

For `CMAKE_SYSTEM_NAME=tvOS`, the project-level Conan graph is intentionally reduced:

- included: `awg-apple/2.0.1`
- included: `libssh/0.11.3@amnezia`
- included: `openssl/3.6.1` with `no_apps=True`
- excluded: `openvpnadapter`
- excluded: `hev-socks5-tunnel`

This keeps the current Apple TV target in the same practical scope as before: app plus WireGuard-only Network Extension.

`libssh` is built with `WITH_EXEC=OFF` on tvOS because tvOS does not provide `fork()` or `execv()`.

## 7. Configure the Project

From the repository root:

```bash
cd "$REPO_ROOT"

"$QT_TVOS_PREFIX/bin/qt-cmake" \
  -B"$BUILD_DIR" \
  -GXcode \
  -DQT_HOST_PATH="$QT_DESKTOP_PREFIX" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_SYSTEM_NAME=tvOS \
  -DCMAKE_OSX_SYSROOT=appletvos
```

If you need to provide an external initialized `client/3rd` tree:

```bash
"$QT_TVOS_PREFIX/bin/qt-cmake" \
  -B"$BUILD_DIR" \
  -GXcode \
  -DQT_HOST_PATH="$QT_DESKTOP_PREFIX" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DCMAKE_SYSTEM_NAME=tvOS \
  -DCMAKE_OSX_SYSROOT=appletvos \
  -DAMNEZIA_THIRDPARTY_ROOT="$AMNEZIA_THIRDPARTY_ROOT"
```

Expected non-fatal configure warnings:

```text
Warning: plug-in QIOSIntegrationPlugin is not known to the current Qt installation.
Warning: plug-in QJpegPlugin is not known to the current Qt installation.
...
```

In this repo those warnings are tolerated because `client/cmake/ios.cmake` also links the static plugin targets explicitly when available.

## 8. Build the Apple TV App

```bash
xcodebuild -quiet \
  -project "$BUILD_DIR/AmneziaVPN.xcodeproj" \
  -scheme AmneziaVPN \
  -configuration RelWithDebInfo \
  -sdk appletvos \
  CODE_SIGNING_ALLOWED=NO \
  build
```

Outputs:

- `$BUILD_DIR/client/RelWithDebInfo-appletvos/AmneziaVPN.app`
- `$BUILD_DIR/client/RelWithDebInfo-appletvos/AmneziaVPN.app/PlugIns/AmneziaVPNNetworkExtension.appex`

Verification:

```bash
file "$BUILD_DIR/client/RelWithDebInfo-appletvos/AmneziaVPN.app/AmneziaVPN"
file "$BUILD_DIR/client/RelWithDebInfo-appletvos/AmneziaVPN.app/PlugIns/AmneziaVPNNetworkExtension.appex/AmneziaVPNNetworkExtension"
lipo -info "$BUILD_DIR/client/RelWithDebInfo-appletvos/AmneziaVPN.app/AmneziaVPN"
lipo -info "$BUILD_DIR/client/RelWithDebInfo-appletvos/AmneziaVPN.app/PlugIns/AmneziaVPNNetworkExtension.appex/AmneziaVPNNetworkExtension"
```

Expected:

```text
Mach-O 64-bit executable arm64
Non-fat file: ... is architecture: arm64
```

Useful plist checks:

```bash
plutil -p "$BUILD_DIR/client/RelWithDebInfo-appletvos/AmneziaVPN.app/Info.plist" | rg 'CFBundleIdentifier|DTPlatformName|UIDeviceFamily|MinimumOSVersion' -C 1
plutil -p "$BUILD_DIR/client/RelWithDebInfo-appletvos/AmneziaVPN.app/PlugIns/AmneziaVPNNetworkExtension.appex/Info.plist" | rg 'CFBundleIdentifier|NSExtension|DTPlatformName|MinimumOSVersion' -C 1
```

Expected:

- `DTPlatformName => appletvos`
- `UIDeviceFamily => 3`
- `MinimumOSVersion => 17.0`
- extension point `com.apple.networkextension.packet-tunnel`

## 9. `.app` vs `.ipa`

This is the normal sequence:

1. `xcodebuild build` -> `.app`
2. `xcodebuild archive` -> `.xcarchive`
3. `xcodebuild -exportArchive` -> `.ipa`

So seeing `.app` after a successful `build` is correct.

## 10. Optional Archive and Export

The commands below are the next step for packaging, but signing and provisioning must be configured first.

Archive:

```bash
xcodebuild \
  -project "$BUILD_DIR/AmneziaVPN.xcodeproj" \
  -scheme AmneziaVPN \
  -configuration RelWithDebInfo \
  -sdk appletvos \
  -archivePath "$BUILD_DIR/AmneziaVPN-tvos.xcarchive" \
  archive
```

Export:

```bash
xcodebuild -exportArchive \
  -archivePath "$BUILD_DIR/AmneziaVPN-tvos.xcarchive" \
  -exportPath "$BUILD_DIR/export-tvos" \
  -exportOptionsPlist /absolute/path/to/ExportOptions.plist
```

The resulting `.ipa` should appear under:

```text
$BUILD_DIR/export-tvos
```

## 11. Known Non-Fatal Warnings

The validated `xcodebuild` still prints warnings that do not break the build:

- missing Swift search path under the active Xcode Metal toolchain
- `SDKROOT[sdk=...]` target-level warnings generated by Xcode project export
- Swift conditional compilation flag warnings such as `GROUP_ID="..."`
- asset catalog warnings because the current icon set is still iOS-shaped, not a full tvOS Top Shelf asset set
- Go/WireGuard umbrella-header warnings from the temporary local `libwg-go.a` bridge
- deprecated libssh SCP API warnings in existing app code
- `qt_import_plugins()` warnings shown during configure

If the static platform plugin is not linked correctly, the typical failure is:

- `_OBJC_CLASS_$_QIOSApplicationDelegate`
- `_qt_main_wrapper`

Those are cleanup tasks, not blockers for the current build proof.

## 12. Fast Rebuild Checklist

If everything is already built once:

1. Reuse `$QT_TVOS_PREFIX`
2. Reuse Conan cache under `$HOME/.conan2`
3. Reuse or pass an initialized `AMNEZIA_THIRDPARTY_ROOT`
4. Re-run `qt-cmake` into `$BUILD_DIR`
5. Re-run `xcodebuild -quiet ... build`
