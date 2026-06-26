# Windows whitelist split-tunnel release build

## Scope

This documents the **include-only (whitelist) app split tunnel** path on Windows: only listed applications use the VPN tunnel; other traffic bypasses the tunnel.

## What must ship together

- **`AmneziaVPN-service.exe`** sends the split policy IOCTL before applying per-app configuration.
- **`mullvad-split-tunnel.sys`** must match the client: it must implement include-only policy and the IOCTL surface the service uses (see code and `recipes/win-split-tunnel` for packaged driver layout).

## Local driver build (optional)

To produce `mullvad-split-tunnel.sys` next to the client for local CMake `POST_BUILD` / `install`, use the scripts under `client/platforms/windows/drivers/` (Visual Studio + WDK). **Kernel-mode and installer Authenticode signing are handled by the official release / CI pipeline**, not by these scripts.

## Installers

Windows installers are produced with the existing CPack / IFW / WiX flow in this repository. Use the same signing and driver packaging steps as for other Amnezia Windows releases once updated driver binaries are integrated into Conan or the build graph.
