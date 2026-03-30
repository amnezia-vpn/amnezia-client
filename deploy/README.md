# AmneziaVPN — Android Build Guide

## Quick start (Docker)

The easiest way to build for Android without configuring a full dev environment.

```bash
# Debug APK for Samsung Galaxy S23+ (arm64-v8a)
./deploy/build_android_docker.sh --apk arm64-v8a --debug

# Release APKs for all architectures
./deploy/build_android_docker.sh --apk all --move

# Release AAB (for Google Play)
./deploy/build_android_docker.sh --aab --move
```

Run from the **repository root**. Build artifacts land in `deploy/build/`.

### Signed release builds

Set these environment variables before running the script:

```bash
export ANDROID_KEYSTORE_PATH=/path/to/android.keystore
export ANDROID_KEYSTORE_KEY_ALIAS=myAlias
export ANDROID_KEYSTORE_KEY_PASS=myPass
./deploy/build_android_docker.sh --apk all --move
```

### Rebuilding the Docker image

The Docker image is built once and cached. Force a rebuild with:

```bash
REBUILD=1 ./deploy/build_android_docker.sh --apk arm64-v8a --debug
```

First build takes 30–60 min (downloads Qt, Android SDK/NDK). Subsequent builds use the cached image and finish in ~10–15 min.

---

## Without Docker

Use `deploy/build_android.sh` directly if you already have the build environment set up.

### Requirements

| Tool | Version |
|------|---------|
| Qt | 6.10.1 |
| Android NDK | r26b (`26.1.10909125`) |
| Android SDK platform | android-36 |
| Android build-tools | 35.0.0 |
| CMake | 3.25+ |
| Go | 1.24.2+ |
| JDK | 17 |

Required Qt modules: `qtremoteobjects qt5compat qtimageformats qtshadertools`

### Environment variables

```bash
export QT_HOST_PATH=/opt/Qt/6.10.1/gcc_64
export ANDROID_SDK_ROOT=/opt/android-sdk
export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/26.1.10909125
```

### Build commands

```bash
# Debug APK for arm64-v8a
./deploy/build_android.sh --apk arm64-v8a --debug

# Release APK for a specific ABI
./deploy/build_android.sh --apk arm64-v8a

# Release APKs for multiple ABIs
./deploy/build_android.sh --apk "arm64-v8a;armeabi-v7a"

# Release APKs for all ABIs
./deploy/build_android.sh --apk all --move

# AAB for Google Play
./deploy/build_android.sh --aab --move

# F-Droid build (unsigned)
./deploy/build_android.sh --apk all --fdroid --move
```

Available ABIs: `arm64-v8a`, `armeabi-v7a`, `x86_64`, `x86`

---

## Installing Qt 6.8+ for Android

`aqtinstall` does not index Qt 6.8+ Android packages because Qt moved them to a
different CDN path (`all_os/android/` instead of `linux_x64/android/`).
Use the bundled script instead:

```bash
python3 deploy/install_qt_android.py 6.10.1 /opt/Qt android_arm64_v8a \
    qtremoteobjects qt5compat qtimageformats qtshadertools
```

Run once for each target architecture:

```bash
for arch in android_arm64_v8a android_armv7 android_x86_64 android_x86; do
    python3 deploy/install_qt_android.py 6.10.1 /opt/Qt $arch \
        qtremoteobjects qt5compat qtimageformats qtshadertools
done
```

Desktop (host) tools still install fine with aqt:

```bash
pip install aqtinstall
aqt install-qt linux desktop 6.10.1 linux_gcc_64 \
    -m qtremoteobjects qt5compat qtimageformats qtshadertools \
    --outputdir /opt/Qt
```

---

## Output locations

| Artifact | Path |
|----------|------|
| Debug APK | `deploy/build/client/android-build/build/outputs/apk/debug/` |
| Release APK | `deploy/build/client/android-build/build/outputs/apk/release/` |
| AAB | `deploy/build/client/android-build/build/outputs/bundle/release/` |
| Moved artifacts (`--move`) | `deploy/build/` |

---

## Tasker / automation support

AmneziaVPN supports Tasker and other Android automation apps.

### Sending commands to AmneziaVPN

Send one of these intents to control the VPN:

| Action | Effect |
|--------|--------|
| `org.amnezia.vpn.tasker.CONNECT` | Connect using the last-used server |
| `org.amnezia.vpn.tasker.DISCONNECT` | Disconnect |
| `org.amnezia.vpn.tasker.TOGGLE` | Toggle connect/disconnect |

Example with `adb`:

```bash
adb shell am broadcast -a org.amnezia.vpn.tasker.CONNECT \
    -p org.amnezia.vpn
```

### Receiving VPN state changes

AmneziaVPN broadcasts `org.amnezia.vpn.tasker.VPN_STATE_CHANGED` whenever the
VPN state changes. In Tasker, use the **Intent Received** event with this action.

Extras included in the broadcast:

| Extra key | Values | Description |
|-----------|--------|-------------|
| `state` | `CONNECTED`, `DISCONNECTED`, `CONNECTING`, `RECONNECTING`, `ERROR` | Current VPN state |
| `server_name` | string | Active server name (empty when disconnected) |
| `protocol` | string | Active protocol label (e.g. `AmneziaWG`) |
