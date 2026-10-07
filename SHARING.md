# AmneziaVPN Share (Windows)

Adds Settings > Tunnel Sharing to AmneziaVPN. Connect XRay, enter a Wi-Fi name
and an 8–63 character password, then enable sharing. Closing the dialog keeps
sharing active. Stop sharing before disconnecting; disconnect also stops it.

The service runs a dedicated TAP-Windows adapter, a userspace TCP/UDP stack and
an authenticated SOCKS5 connection to the running XRay instance. Windows
Mobile Hotspot supplies Wi-Fi and DHCP. The service routes client IPv4 traffic
through TAP to XRay and returns replies to the hotspot; it does not create a
separate WinNAT instance. The existing VPN Kill Switch is retained, with a
temporary firewall rule for the sharing path. XRay credentials travel through
child-process stdin, not argv or configuration files. IPv4 TCP/UDP is the
supported data path; ICMP and IPv6 sharing are not promised.

## Build

Build tools/tapbridge with the Go version required by go.mod using build.ps1,
then configure and build the regular Windows MSVC/Qt client and service and run
cmake --install. Place tapctl.exe from the matching OpenVPN Windows build in
service/server/sharing before building. The TAP-Windows driver must already be
installed (the regular package includes it). Distribute the complete install
tree, not only AmneziaVPN.exe.

## Checks performed on this machine

- Authenticated SOCKS5 TCP request and UDP association through the TAP stack.
- Clean bridge termination when its owner closes stdin.
- WinRT hotspot start/stop from LocalSystem session 0.

End-to-end hotspot sharing was also confirmed on a connected client device.

## Dependencies

The helper uses github.com/xjasonlyu/tun2socks/v2 and github.com/songgao/water;
their MIT license texts are in tools/tapbridge. Other transitive dependencies
and pinned versions are in go.mod/go.sum. tapctl is supplied by OpenVPN under
its license and is intentionally not included in this source change. AmneziaVPN
retains its upstream license and attribution.
