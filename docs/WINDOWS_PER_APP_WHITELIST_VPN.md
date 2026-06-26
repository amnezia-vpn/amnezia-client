# Windows: per-app whitelist VPN (“only listed .exe through VPN”) — spike

Date: 2026-05-13. Scope: assess feasibility **without** a new kernel `.sys` driver, using existing usermode WFP in the Windows daemon (`client/platforms/windows/daemon/windowsfirewall.cpp`, `windowsdaemon.cpp`), compatible with WireGuard **`Table = off`** in `client/platforms/windows/daemon/wireguardutilswindows.cpp`.

## Verdict

**Not viable** as a complete solution using **only** the current style of WFP filters (permit/block on ALE classify layers) in this codebase.

WFP can **authorize or deny** connections (`FWPM_LAYER_ALE_AUTH_CONNECT_V4` / `…_V6`, etc.). It does **not** choose which physical or tunnel interface carries a socket’s traffic. On Windows, outbound interface selection for arbitrary processes follows the **routing table**, interface binding, and mechanisms such as the existing **Mullvad split-tunnel driver** (`windowssplittunnel.cpp`, `\\.\MULLVADSPLITTUNNEL`), which is **exclude-from-VPN by design** (inverse of a strict per-app whitelist).

Therefore **`AppsRouteMode::VpnOnlyForwardApps`** cannot be honestly implemented here by “just adding WFP rules” while leaving routing as today’s full-tunnel / driver-exclude model.

**Next steps (structured paths):** see **[`WINDOWS_PER_APP_WHITELIST_ROADMAP.md`](WINDOWS_PER_APP_WHITELIST_ROADMAP.md)** — Path **D** (driver fork / inclusion), **P** (proxy/shim), **R** (routing-only hybrid), **U** (WFP + redirect / callout clarification).

## Evidence in tree

- **`wireguardutilswindows.cpp`**: sets `extraConfig["Table"] = "off"`; brings up tunnel; uses **`WindowsRouteMonitor`** and **`CreateIpForwardEntry2`** / `DeleteIpForwardEntry2` for prefixes (`updateRoutePrefix`, `addExclusionRoute`, etc.). Routing, not WFP, steers traffic toward the tunnel when prefixes match.
- **`windowsfirewall.cpp`**: kill switch and exceptions — `blockTrafficTo`, `allowTrafficOfAdapter`, `allowTrafficForAppOnAll`, DNS allows, port-53 block, etc. All are **filter** actions, not path redirection to another NIC.
- **`windowsdaemon.cpp`**: app “split” uses **`WindowsSplitTunnel::excludeApps`** only when `m_vpnDisabledApps` is non-empty — driver path, not WFP whitelist.
- **`client/core/utils/routeModes.h`**: defines `AppsRouteMode::VpnOnlyForwardApps` at the type level; **no Windows backend** ties this enum to a working whitelist tunnel in this spike.

## Leak / behavior risks (if someone still layered WFP on top)

- **DNS / `svchost.exe`**: system DNS often runs in **`svchost`**, not in the browser’s `.exe`. App-scoped WFP conditions (`FWPM_CONDITION_ALE_APP_ID`) do not attribute those queries to “the app you care about”. Kill switch already uses broad **block UDP/TCP remote port 53** plus explicit allows — easy to mis-handle in a hybrid whitelist design.
- **IPv6**: `allowTrafficForAppOnAll` in `windowsfirewall.cpp` only installs filters on **`FWPM_LAYER_ALE_AUTH_CONNECT_V4`** and **`…_RECV_ACCEPT_V4`** (no v6 layers for that helper). Any whitelist story must treat IPv6 explicitly or accept asymmetry.
- **Kill switch interaction**: `enableInterface` / `enablePeerTraffic` combine global blocks and high-weight permits. Adding overlapping per-app rules requires careful **weight** ordering; mistakes **brick** general connectivity or **leak** when permits are too broad.

## Practical alternatives (soft fallbacks)

High-level options are expanded in **[`WINDOWS_PER_APP_WHITELIST_ROADMAP.md`](WINDOWS_PER_APP_WHITELIST_ROADMAP.md)** (scope, risks, order of work). In short:

1. **Path R — Site / prefix split tunnel** (already supported via `allowedIPAddressRanges` / excluded routes in JSON) — not per-app, but honest and driver-free where routing suffices.
2. **Path P — Application-level proxy** (SOCKS5/HTTP on loopback) — per-app only if the app supports proxy settings; see roadmap for limitations.
3. **Path D / U — Kernel component**: fork or extend split-tunnel driver for **include** semantics (**Path D**), or a smaller **WFP callout** redirect driver (**Path U** — still kernel-signed). Spike remains code-review only; actionable breakdown is in the roadmap.

## Driver fork (outline only)

If pursuing a driver later: duplicate Mullvad-style IOCTL surface or design a new one; implement **PID / image path → bind / redirect** for **inclusion** list; coordinate with `Table=off` routes so default path stays on physical NIC while included processes’ traffic is forced onto the tunnel interface. Requires signing, CI for `sys`, and tight review of **rebind** / **child process** / **UWP** edge cases. **Details:** [`WINDOWS_PER_APP_WHITELIST_ROADMAP.md`](WINDOWS_PER_APP_WHITELIST_ROADMAP.md) (Path D signing note: verify Microsoft policy at implementation time).

## QML / UI

This `dev` tree is mostly **Qt widgets / C++**, not QML, for the main client shell. **`AppsRouteMode`** is not wired to a Windows-specific QML toggle here. Any future UI for whitelist mode should stay **hidden or explicitly “experimental”** until a backend (driver or redirect) exists.

## Build notes

Spike was **code review only**; full Windows client build (Qt, Conan, etc.) was **not** executed in this session.
