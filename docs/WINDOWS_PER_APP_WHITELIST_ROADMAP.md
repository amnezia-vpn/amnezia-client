# Windows per-app whitelist VPN — implementation roadmap (post-spike)

**Date:** 2026-05-13. **Related:** [`WINDOWS_PER_APP_WHITELIST_VPN.md`](WINDOWS_PER_APP_WHITELIST_VPN.md) (spike verdict: usermode-only WFP is insufficient for true “only these `.exe` through the VPN”).

This document expands **actionable implementation paths** after the spike. Each path lists **scope**, **risks**, and a **rough order of work**. Pick based on product guarantees (exe parity vs “good enough” routing/proxy).

---

## Path D — Driver fork (split tunnel, inclusion mode)

**Idea:** Fork or extend the Mullvad-style Windows split-tunnel kernel driver (upstream reference: [`mullvad/win-split-tunnel`](https://github.com/mullvad/win-split-tunnel)), which today powers **exclude-from-VPN** in this tree (`windowssplittunnel.cpp`, device `\\.\MULLVADSPLITTUNNEL`). Add an **inclusion** mode (only listed processes / image paths are forced onto the tunnel; everything else stays on the default path), aligned with `Table = off` routing in `wireguardutilswindows.cpp`.

| | |
| --- | --- |
| **Scope** | Kernel `.sys` + usermode IOCTL client: reverse semantics vs current exclude list; PID / image path / stable app identity (incl. UWP, elevation, job objects) as needed. **Amnezia integration:** `windowssplittunnel.cpp`, `windowsdaemon.cpp`, `InterfaceConfig` / settings model, client JSON (schema + migration), installer packaging of the signed driver. CI that builds/signs the driver artifact and runs smoke tests on Windows runners or manual matrix. |
| **Risks** | Long-term maintenance vs upstream Mullvad drift; subtle **rebind**, **child process**, **shared service** (e.g. DNS in `svchost.exe`), and **IPv6** parity; incorrect rules can **brick** connectivity or **leak** traffic. **Signing / distribution:** kernel-mode code requires a compliant signing and Microsoft acceptance workflow. |
| **Rough order** | (1) Requirements: inclusion semantics + interaction with kill switch and routes. (2) Fork driver repo; map existing IOCTL surface. (3) Implement inclusion path in driver; extend or version IOCTLs (backward compatibility if shipping alongside exclude mode). (4) Wire daemon: split tunnel module + config flags. (5) Client JSON + UI (likely experimental until stable). (6) **Signing pipeline** (see below). (7) QA matrix: reboot, sleep, fast user switching, updater, games, browsers, system proxy. |

### Signing pipeline (verify at implementation time)

Kernel drivers on Windows require **Authenticode** signing; **attestation signing** / **Microsoft Hardware Dev Center** policies evolve. As of the **2025–2026** timeframe, teams typically need:

- An **EV code signing certificate** for the organization.
- Submission through the **Hardware Dev Center** (or then-current equivalent) for **Microsoft signature** / attestation, subject to **current Microsoft policy** for the driver class and OS versions you target.

**Action:** Before committing to Path D, **re-verify** the official Microsoft documentation and partner requirements for kernel driver signing and dashboard submission at **implementation time** (policy and UI change frequently).

---

## Path P — Proxy / shim (loopback SOCKS or HTTP)

**Idea:** Run a **local SOCKS5 or HTTP proxy** on `127.0.0.1` whose upstream is carried **inside the VPN** (or is the tunnel endpoint). Only applications **configured** to use that proxy send traffic through the tunnel leg; others use normal routing.

| | |
| --- | --- |
| **Scope** | Documented user workflow; optional small helper to launch apps with `HTTP(S)_PROXY` / SOCKS env vars; optional in-client “local proxy” feature if product wants first-party UX. No kernel driver if users manually configure supported apps. |
| **Risks** | **Not all apps** respect WinINET / system proxy or env-based proxy settings. **No transparent per-exe** guarantee without an additional redirect layer (which moves toward Path U or D). **DNS** may still bypass the app’s proxy depending on resolver and Windows DNS stack. |
| **Rough order** | (1) Document supported scenarios (browsers, CLI tools, some IDEs). (2) Define a standard local port and auth model. (3) Optional: launcher script / client menu “Open app via VPN proxy”. (4) Security review: bind to loopback only; avoid accidental LAN exposure. |

---

## Path R — Routing-only hybrid (strict non–default-route VPN + IP allowlists)

**Idea:** Avoid “per exe” entirely: use **routing policy** so the VPN is **not** the default route, and push **only selected prefixes** (or “allowed IP ranges” already in JSON) through the tunnel. Matches destinations, not processes.

| | |
| --- | --- |
| **Scope** | Leverage / extend existing split-tunnel concepts (`allowedIPAddressRanges`, excluded routes, `Table = off` behavior) and document when this satisfies the threat model (e.g. “only these subnets use VPN”). |
| **Risks** | **No exe parity:** any process talking to an allowed prefix uses the tunnel; other apps to the same IPs do too. **Dynamic IPs** and **CDN fronting** complicate allowlists. |
| **Rough order** | (1) Product wording: “site/IP split”, not “app whitelist”. (2) Harden docs + defaults for leak testing. (3) Optional automation: curated prefix lists with update channel. |

---

## Path U — Usermode WFP + redirect (clarification and minimal kernel)

**Idea (clarification):** The spike showed that **generic WFP permit/block filters** on ALE classify layers **authorize or deny** flows; they do **not** assign outbound traffic to a **specific network interface** or rewrite path selection for arbitrary sockets. Interface selection remains **routing**, **bind()**, and **driver-assisted** mechanisms (see spike doc).

| | |
| --- | --- |
| **Scope** | **Education + narrow POC:** document why `AppsRouteMode::VpnOnlyForwardApps` cannot be completed by WFP rules alone. If true redirection is required at the forward path, a **WFP callout** or similar **still loads a kernel driver** (minimal `.sys` with callout vs a **full** split-tunnel fork). |
| **Risks** | A callout driver is **smaller surface** than forking all of Mullvad split tunnel, but you still face **signing**, **performance**, **upgrade rollbacks**, and **interaction with other WFP providers** (antivirus, other VPNs). |
| **Rough order** | (1) Architecture note in daemon: WFP layers used today vs needed redirect layers. (2) Spike/prototype only if product commits: callout inspect + redirect policy by app ID (with threat model). (3) Compare effort vs Path D (full inclusion driver may reuse more existing Amnezia code paths). |

---

## Path comparison (one glance)

| Path | Exe-level “whitelist” | Kernel code | Typical user friction |
| --- | --- | --- | --- |
| **D** | Yes (if driver implements it) | Yes (full split-tunnel style) | Low after install |
| **P** | Only for cooperating apps | No | Medium–high (per-app settings) |
| **R** | No (IP/site based) | Uses existing model | Low |
| **U** | Possible with callout | Yes (minimal driver) | Low after install; high dev risk |

---

## Cross-links

- Spike (evidence, verdict): [`WINDOWS_PER_APP_WHITELIST_VPN.md`](WINDOWS_PER_APP_WHITELIST_VPN.md)
- Fork note in root README: [`../README.md`](../README.md) (workspace fork note block)
