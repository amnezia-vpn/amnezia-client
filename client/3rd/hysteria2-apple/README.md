# hysteria2-apple

Go CGo wrapper around the [Hysteria2](https://github.com/apernet/hysteria) client
for iOS (Network Extension).

## Architecture

```
PacketTunnelProvider+Hysteria2.swift
  │  reads Hysteria2Config, generates YAML
  │  calls LibHysteria2SetSockCallback  ──► binds UDP sockets to physical iface
  │  calls LibHysteria2RunClient(configPath)
  │    └─► starts Hysteria2 QUIC client + SOCKS5 proxy on [::1]:10808
  │
  └─► hev-socks5-tunnel (HevSocks5Tunnel.xcframework)
        bridges the TUN device → SOCKS5 proxy
```

## Building

Prerequisites:
- Go 1.21+
- Xcode / Command Line Tools
- `go mod download` (downloads Hysteria2 dependencies)

```bash
cd client/3rd/hysteria2-apple
go mod download
./build-ios.sh
```

Output: `client/3rd-prebuilt/3rd-prebuilt/hysteria2/Hysteria2.xcframework`

## Config format (YAML written by Swift)

```yaml
server: example.com:443
auth: your_password
bandwidth:
  up: 20 mbps
  down: 100 mbps
tls:
  sni: example.com
  insecure: false
socks5:
  listen: "[::1]:10808"
# Optional obfuscation:
obfs:
  type: salamander
  salamander:
    password: obfs_password
```

## Socket protection (iOS-specific)

iOS requires that sockets created by the Hysteria2 client (QUIC/UDP) are
explicitly bound to the **physical** network interface, not the VPN TUN
interface.  If they use the TUN interface, Hysteria2's own traffic would be
routed through itself, creating a loop.

`LibHysteria2SetSockCallback` registers a callback that is invoked each time
the Hysteria2 client creates a UDP socket.  The iOS Network Extension uses
`setsockopt(IP_BOUND_IF)` and `setsockopt(IPV6_BOUND_IF)` to bind the socket
to the currently active physical interface (Wi-Fi / cellular).
