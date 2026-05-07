# Local gateway: LAN / Wi‑Fi (macOS host ↔ iOS client)

This document is the **implementation checklist** for `tools/local_gateway` plus the **AmneziaVPN client** dev flags used with a **plaintext** mock over **private LAN** addresses (not only `127.0.0.1`).

## Goals

1. Run **`tools/local_gateway`** on a Mac; reach it from an iPhone on the same Wi‑Fi via **`http://<mac-lan-ip>:<port>/`**.
2. **`POST /v1/updater_endpoint`** must return a **`url`** the phone can reach (not `http://127.0.0.1:8080`, which points at the phone itself).
3. **Verbose logs** on the server for debugging.
4. **Client:** plaintext JSON to the mock only for **loopback** by default; **optional** plaintext to **RFC1918 / ULA / link-local** hosts when **`AMNEZIA_LAN_PLAINTEXT_GATEWAY=ON`** (requires **`AMNEZIA_QR_PAIRING_ALLOW=ON`**).

> **Security:** `AMNEZIA_LAN_PLAINTEXT_GATEWAY` disables transport encryption to any configured gateway whose host parses as a private LAN address. Use **only** in internal dev builds with `tools/local_gateway`, never for production endpoints.

---

## Phase A — Go server (`main.go`)

| Item | Status |
|------|--------|
| **`-listen` / `LOCAL_GATEWAY_LISTEN`** | Default `0.0.0.0:8080`; append `:8080` if port omitted. Still **`tcp4`** only (macOS LAN / curl oddities). |
| **`-public-base` / `LOCAL_GATEWAY_PUBLIC_BASE`** | Base URL **without** trailing slash; fed into **`POST /v1/updater_endpoint`** as `{"url":…}`. |
| **`-auto-public`** (default true) | If `public-base` empty, pick first **non-loopback IPv4** (prefers **private** / link-local over public). |
| **`-pairing-ttl` / `-long-poll` / `-rate-limit-excess-after`** | Replace hardcoded **120s** / **120s** / **0** for pairing and Free mock. |
| **Startup banner** | Logs listen address, chosen **`publicUpdaterBaseURL`**, and **every** `http://<ipv4>:port/` candidate per interface. |
| **Request logging** | `REQ start` (remote, method, path, query, UA, `X-Client-Request-ID`, content type/length) + `REQ end` (status, duration). |
| **Pairing logs** | Extra fields on register/complete (`installation_uuid` short, app/os, `config_len`, protocol count). |

### Example: Mac + iPhone

```bash
cd tools/local_gateway
# Explicit base (safest if you have several NICs):
LOCAL_GATEWAY_PUBLIC_BASE='http://192.168.1.10:8080' go run -buildvcs=false .

# Or rely on auto-public (first suitable IPv4 + listen port):
go run -buildvcs=false .
```

Firewall: allow incoming TCP on the chosen port (e.g. **8080**) for **local subnet**.

---

## Phase B — Client (CMake + C++)

| CMake | Meaning |
|-------|---------|
| **`AMNEZIA_QR_PAIRING_ALLOW=ON`** | Enables QR pairing + **loopback** plaintext to `localhost` / `127.0.0.1` / `::1` (`GatewayController`). |
| **`AMNEZIA_LAN_PLAINTEXT_GATEWAY=ON`** | **Also** sends plaintext JSON when gateway host is **private LAN** per `NetworkUtilities::hostIsPrivateLanAddress` (IPv4: `10/8`, `172.16/12`, `192.168/16`, `169.254/16`; IPv6: `fe80::/10`, `fc00::/7`). **Requires** `AMNEZIA_QR_PAIRING_ALLOW=ON` or CMake **fatal error**. |

| Code | Change |
|------|--------|
| `NetworkUtilities::hostIsPrivateLanAddress` | Shared predicate for gateway + pairing + sandbox endpoint retention. |
| `GatewayController::prepareRequest` | Plaintext path for LAN when `AMNEZIA_LAN_PLAINTEXT_GATEWAY` is defined. |
| `PairingController::isLocalGatewayHost` | Treats LAN like mock for **120s** long-poll alignment with `tools/local_gateway`. |
| `SecureAppSettingsRepository::getGatewayEndpoint(true)` | Keeps user’s LAN mock URL under **test purchase** (same idea as loopback). |

Configure iOS dev build with both options, set gateway in app to **`http://<mac-ip>:8080/`** (trailing slash as today).

---

## Phase C — Smoke test

```bash
cd tools/local_gateway
go run -buildvcs=false . &
sleep 1
bash verify.sh 'http://127.0.0.1:8080'
# or against LAN IP from another shell on same machine:
bash verify.sh 'http://192.168.1.10:8080'
```

---

## References

- `tools/local_gateway/README.md` — quick start and endpoint table.
- `docs/local-gateway-mock.md` — client wiring and checklist.
