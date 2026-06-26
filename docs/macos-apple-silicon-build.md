# macOS Apple Silicon build

This checkout can build a macOS app bundle with an `arm64` slice using Qt 6.10
and Conan.

## Requirements

- Xcode with the macOS SDK.
- Qt 6.10.3 or newer installed at `~/Qt/6.10.3`, or pass `QT_ROOT_PATH`.
- Python 3 for the project-local Conan virtual environment.

The helper script creates `.venv`, installs Conan when needed, initializes
submodules, builds the desktop macOS target, deploys Qt frameworks into the app
bundle, embeds `AmneziaVPN-service` and macOS `pf` data, removes SQL plugins with
external Homebrew/Postgres dependencies, and ad-hoc signs the result.

```sh
QT_ROOT_PATH="$HOME/Qt/6.10.3" ./deploy/build_macos_apple_silicon.sh
```

By default the script requests a pure Apple Silicon build:

```sh
MACOS_ARCHS=arm64 ./deploy/build_macos_apple_silicon.sh
```

For a universal local build:

```sh
MACOS_ARCHS="arm64;x86_64" ./deploy/build_macos_apple_silicon.sh
```

The output app is:

```text
deploy/build/client/AmneziaVPN.app
```

The script verifies that both `AmneziaVPN` and `AmneziaVPN-service` contain an
`arm64` slice.

## Signing note

The helper uses ad-hoc signing so the bundle can be validated locally. It does
not produce a Developer ID signed or notarized distribution build. Network
Extension builds still require Apple Developer entitlements and provisioning.
