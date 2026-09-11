#!/usr/bin/env bash
# Run Android tests locally on a connected device or emulator.
#
# ABI guidance:
#   x86_64  (default) – use for emulators; HVF/Rosetta hardware acceleration works on
#                       both Intel and Apple Silicon Macs, so OpenGL doesn't crash.
#   arm64-v8a         – use for physical ARM devices only; arm64-v8a emulators on
#                       Apple Silicon have OpenGL emulation bugs that crash Qt apps.
#
# Prerequisites:
#   - Qt for Android x86_64 (or arm64-v8a for devices) installed via Qt Maintenance Tool
#   - Android SDK + NDK available via Android Studio
#   - `adb devices` shows at least one device/emulator
#   - Conan 2 installed  (pip install "conan==2.28.0")
#
# Usage:
#   bash run_android_tests.sh                         # x86_64 emulator (default)
#   bash run_android_tests.sh --abi arm64-v8a         # physical ARM device
#   bash run_android_tests.sh --test openssl          # only openssl test
#   bash run_android_tests.sh --test restore_backup   # only restore-backup test
#   bash run_android_tests.sh --wait-boot             # wait for emulator to boot first

set -euo pipefail

# ── Configurable defaults ──────────────────────────────────────────────────────
QT_ROOT="${QT_ROOT:-/Users/nickpc/Qt/6.10.1}"
ANDROID_SDK="${ANDROID_HOME:-/Users/nickpc/Library/Android/sdk}"
NDK_VERSION="${NDK_VERSION:-27.3.13750724}"
ANDROID_PLATFORM="${ANDROID_PLATFORM:-android-28}"
# x86_64: recommended for emulators (HVF/Rosetta hardware acceleration works, no GL crashes)
# arm64-v8a: for physical ARM devices only (arm64 emulators have OpenGL bugs on Apple Silicon)
ABI="${ABI:-x86_64}"
ABI_DIR="${ABI//-/_}"
BUILD_DIR="${BUILD_DIR:-build/android_${ABI_DIR}}"
TEST_FILTER="all"   # "all" | "openssl" | "restore_backup"
WAIT_BOOT=false     # set to true or pass --wait-boot to wait for emulator startup
DEVICE_SERIAL=""    # specific device serial; empty = use ANDROID_SERIAL or first device
# ──────────────────────────────────────────────────────────────────────────────

# Parse CLI flags
while [[ $# -gt 0 ]]; do
  case $1 in
    --abi)        ABI="$2";         ABI_DIR="${ABI//-/_}"; BUILD_DIR="build/android_${ABI_DIR}"; shift 2 ;;
    --test)       TEST_FILTER="$2"; shift 2 ;;
    --wait-boot)  WAIT_BOOT=true;   shift ;;
    --device)     DEVICE_SERIAL="$2"; shift 2 ;;
    --list-devices) adb devices -l; exit 0 ;;
    *) echo "Unknown option: $1"; exit 1 ;;
  esac
done

# Apply device selection
if [[ -n "$DEVICE_SERIAL" ]]; then
  export ANDROID_SERIAL="$DEVICE_SERIAL"
  echo ">>> Using device: $ANDROID_SERIAL"
elif [[ -n "${ANDROID_SERIAL:-}" ]]; then
  echo ">>> Using device from env: $ANDROID_SERIAL"
fi

# Map ABI to Qt arch directory name
case "$ABI" in
  arm64-v8a)  QT_ARCH_DIR="android_arm64_v8a" ;;
  x86_64)     QT_ARCH_DIR="android_x86_64"    ;;
  armeabi-v7a) QT_ARCH_DIR="android_armv7"    ;;
  x86)        QT_ARCH_DIR="android_x86"       ;;
  *) echo "Unsupported ABI: $ABI"; exit 1 ;;
esac

QT_ANDROID="$QT_ROOT/$QT_ARCH_DIR"
QT_HOST="$QT_ROOT/macos"
NDK_PATH="$ANDROID_SDK/ndk/$NDK_VERSION"

echo "=== Android test runner (local) ==="
echo "  ABI           : $ABI"
echo "  Qt Android    : $QT_ANDROID"
echo "  Qt Host       : $QT_HOST"
echo "  NDK           : $NDK_PATH"
echo "  Build dir     : $BUILD_DIR"
echo "  Platform      : $ANDROID_PLATFORM"
echo ""

# Sanity checks
[[ -d "$QT_ANDROID" ]] || { echo "ERROR: Qt Android not found at $QT_ANDROID"; exit 1; }
[[ -d "$NDK_PATH"   ]] || { echo "ERROR: NDK not found at $NDK_PATH";           exit 1; }
command -v adb >/dev/null  || { echo "ERROR: adb not found in PATH";             exit 1; }
if $WAIT_BOOT; then
  echo ">>> Waiting for emulator to finish booting..."
  adb wait-for-device shell 'while [[ -z $(getprop sys.boot_completed) ]]; do sleep 1; done'
  echo "    Emulator ready."
fi

# List connected devices for visibility
echo ">>> Connected devices:"
adb devices -l | tail -n +2 | grep -v "^$" || true
echo ""

adb devices | grep -qE "device$|emulator" || {
  echo "ERROR: No Android device/emulator connected."
  echo "  Run:  bash run_android_tests.sh --list-devices"
  echo "  Then: bash run_android_tests.sh --device <serial>"
  exit 1
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# ── Locate cmake ──────────────────────────────────────────────────────────────
if ! command -v cmake &>/dev/null; then
  QT_CMAKE="/Users/nickpc/Qt/Tools/CMake/CMake.app/Contents/bin"
  if [[ -x "$QT_CMAKE/cmake" ]]; then
    export PATH="$QT_CMAKE:$PATH"
    echo ">>> Using Qt-bundled cmake: $QT_CMAKE/cmake"
  else
    echo "ERROR: cmake not found. Install it or add it to PATH."
    exit 1
  fi
fi

# ── Locate ninja ──────────────────────────────────────────────────────────────
if ! command -v ninja &>/dev/null; then
  QT_NINJA="/Users/nickpc/Qt/Tools/Ninja"
  if [[ -x "$QT_NINJA/ninja" ]]; then
    export PATH="$QT_NINJA:$PATH"
    echo ">>> Using Qt-bundled ninja: $QT_NINJA/ninja"
  else
    echo "ERROR: ninja not found. Install it (brew install ninja) or add it to PATH."
    exit 1
  fi
fi

# ── Add NDK toolchain to PATH (provides clang / clang++) ─────────────────────
NDK_TOOLCHAIN="$NDK_PATH/toolchains/llvm/prebuilt/darwin-x86_64/bin"
if [[ -d "$NDK_TOOLCHAIN" ]]; then
  export PATH="$NDK_TOOLCHAIN:$PATH"
fi

# ── Required env vars for androiddeployqt ────────────────────────────────────
export ANDROID_NDK_ROOT="$NDK_PATH"
export ANDROID_SDK_ROOT="$ANDROID_SDK"

# Auto-detect JAVA_HOME (Android Studio or system JDK)
if [[ -z "${JAVA_HOME:-}" ]]; then
  for candidate in \
    "/Applications/Android Studio.app/Contents/jbr/Contents/Home" \
    "/Applications/Android Studio.app/Contents/jre/Contents/Home" \
    "$(/usr/libexec/java_home 2>/dev/null || true)"
  do
    if [[ -x "$candidate/bin/java" ]]; then
      export JAVA_HOME="$candidate"
      break
    fi
  done
fi
[[ -n "${JAVA_HOME:-}" ]] || { echo "ERROR: JAVA_HOME not set and no JDK found. Install Android Studio or JDK 17."; exit 1; }
echo ">>> JAVA_HOME: $JAVA_HOME"

# ── Configure ─────────────────────────────────────────────────────────────────
# Qt's own toolchain file sets CMAKE_PREFIX_PATH, ABI, NDK, and sysroot
# correctly. Using raw CMake Android variables causes Qt6 to not be found
# because the NDK toolchain resets find paths.
QT_TOOLCHAIN="$QT_ANDROID/lib/cmake/Qt6/qt.toolchain.cmake"
[[ -f "$QT_TOOLCHAIN" ]] || { echo "ERROR: Qt toolchain not found: $QT_TOOLCHAIN"; exit 1; }

# Detect the actual NDK host tag from what's present in the NDK
# (Apple Silicon Macs have only darwin-x86_64 in NDK ≤27, not darwin-aarch64)
NDK_HOST_TAG=""
for candidate in darwin-x86_64 darwin-aarch64 linux-x86_64; do
  if [[ -d "$NDK_PATH/toolchains/llvm/prebuilt/$candidate" ]]; then
    NDK_HOST_TAG="$candidate"
    break
  fi
done
[[ -n "$NDK_HOST_TAG" ]] || { echo "ERROR: Cannot detect NDK host tag in $NDK_PATH/toolchains/llvm/prebuilt/"; exit 1; }
echo ">>> NDK host tag: $NDK_HOST_TAG"

# ABI → sysroot triple mapping for libc++_shared.so
case "$ABI" in
  arm64-v8a)   SYSROOT_TRIPLE="aarch64-linux-android" ;;
  x86_64)      SYSROOT_TRIPLE="x86_64-linux-android"  ;;
  armeabi-v7a) SYSROOT_TRIPLE="arm-linux-androideabi"  ;;
  x86)         SYSROOT_TRIPLE="i686-linux-android"     ;;
esac
# androiddeployqt appends "/<triple>/libc++_shared.so" itself, so point to usr/lib/, not usr/lib/<triple>/
STD_CXX_PATH="$NDK_PATH/toolchains/llvm/prebuilt/$NDK_HOST_TAG/sysroot/usr/lib"

echo ">>> Configuring..."
cmake -S . -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$QT_TOOLCHAIN" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
  -DANDROID_SDK_ROOT="$ANDROID_SDK" \
  -DCMAKE_ANDROID_NDK="$NDK_PATH" \
  -DCMAKE_ANDROID_NDK_TOOLCHAIN_HOST_TAG="$NDK_HOST_TAG" \
  -DQT_HOST_PATH="$QT_HOST" \
  -DQt6_DIR="$QT_ANDROID/lib/cmake/Qt6" \
  -DCMAKE_PREFIX_PATH="$QT_ANDROID"

# Patch deployment settings JSONs: cmake may leave ndk-host empty on Apple Silicon
for json_file in "$BUILD_DIR"/tests/android-*-deployment-settings.json; do
  [[ -f "$json_file" ]] || continue
  python3 - "$json_file" "$NDK_HOST_TAG" "$STD_CXX_PATH" <<'PYEOF'
import sys, json
path, host_tag, stl_path = sys.argv[1], sys.argv[2], sys.argv[3]
with open(path) as f:
    d = json.load(f)
changed = False
if not d.get("ndk-host"):
    d["ndk-host"] = host_tag
    changed = True
if d.get("stdcpp-path", "").startswith("/usr/lib/") or "/usr/lib//" in d.get("stdcpp-path", ""):
    d["stdcpp-path"] = stl_path + "/"
    changed = True
if changed:
    with open(path, "w") as f:
        json.dump(d, f, indent=3)
    print(f"  Patched {path}")
PYEOF
done

ADB_BIN=$(which adb)

run_test() {
  local target="$1"
  local apk_dir="$BUILD_DIR/tests/android-build"
  local apk="$apk_dir/${target}.apk"
  local pkg_name="org.qtproject.example.${target}"
  local activity="${pkg_name}/org.qtproject.qt.android.bindings.QtActivity"

  echo ""
  echo ">>> Building $target APK..."
  cmake --build "$BUILD_DIR" --target "${target}_make_apk"

  echo ">>> Installing $target APK on device..."
  "$ADB_BIN" install -r "$apk"

  # Pre-create files/stdout.txt before launching so the test process finds it
  # immediately and androidtestrunner-style tail won't fail on a missing file.
  "$ADB_BIN" shell "run-as $pkg_name sh -c 'mkdir -p files && : > files/stdout.txt'" 2>/dev/null || true
  # Remove stale exit code from any previous run
  "$ADB_BIN" shell "run-as $pkg_name sh -c 'rm -f files/qtest_last_exit_code'" 2>/dev/null || true

  echo ">>> Launching $target..."
  "$ADB_BIN" shell am start -n "$activity"

  # Poll for qtest_last_exit_code (written by Qt's test runner on exit)
  echo ">>> Waiting for test to finish (up to 120s)..."
  local elapsed=0 exit_code=""
  while [[ $elapsed -lt 120 ]]; do
    sleep 2
    elapsed=$((elapsed + 2))
    exit_code=$("$ADB_BIN" shell \
      "run-as $pkg_name cat files/qtest_last_exit_code 2>/dev/null" \
      | tr -d '[:space:]' || true)
    [[ -n "$exit_code" ]] && break
  done

  echo ""
  local stdout_content
  stdout_content=$("$ADB_BIN" shell \
    "run-as $pkg_name cat files/stdout.txt 2>/dev/null" || true)
  if [[ -n "$stdout_content" ]]; then
    echo ">>> Test output (files/stdout.txt):"
    echo "$stdout_content"
  fi

  if [[ -z "$exit_code" ]]; then
    echo ">>> TIMEOUT: $target did not complete within 120s."
    exit 1
  elif [[ "$exit_code" != "0" ]]; then
    echo ">>> $target FAILED (exit code: $exit_code)"
    exit 1
  else
    echo ">>> $target PASSED"
  fi
}

# ── Run tests ─────────────────────────────────────────────────────────────────
case "$TEST_FILTER" in
  all)
    run_test test_android_openssl
    run_test test_android_restore_backup
    ;;
  openssl)
    run_test test_android_openssl
    ;;
  restore_backup)
    run_test test_android_restore_backup
    ;;
  *)
    echo "Unknown --test value: $TEST_FILTER (use 'all', 'openssl', or 'restore_backup')"
    exit 1
    ;;
esac

echo ""
echo "=== Done ==="
