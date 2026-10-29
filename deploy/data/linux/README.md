**English** | [Русский](README.ru.md)

---

# AmneziaVPN on Linux without systemd

Run the official **AmneziaVPN** client on Gentoo/OpenRC, Void/runit, Artix,
Alpine, Devuan, Slackware, s6, dinit and any other init system.

The privileged daemon is a normal Qt process and does not need systemd. This
change adds the missing native pieces:

- native DNS configuration when `systemd-resolved` is absent;
- native service scripts for every supported init system;
- clean shutdown on `SIGTERM` and firewall cleanup on stop;
- no Python, no `dbus-python`, no external DNS bridge.

---

## Why this is needed

systemd is used in only a few places:

| Component | Location | Fix |
|---|---|---|
| Service supervision | `post_install.sh`, `systemctl` | native init scripts + init detection |
| DNS configuration | `DnsUtilsLinux` — D-Bus `org.freedesktop.resolve1` | native `resolvconf`/`/etc/resolv.conf` fallback |
| DNS cache flush | `RouterLinux::flushDns()` — `systemctl restart nscd/systemd-resolved` | `nscd -i hosts`, otherwise a no-op |
| Privileged `ip` calls | `RouterLinux` — `sudo ip ...` | run directly when already root |
| Clean shutdown | `SIGTERM` was not handled | instantiate `SignalHandler` |

The DNS problem is the critical one: on connect the daemon installs an
`iptables` chain (`amnvpn.a.310.blockDNS`) that rejects port 53 outside the
tunnel. If the resolver cannot be reconfigured, all DNS breaks — the classic
"VPN connected but no Internet".

## Native DNS fallback

`DnsUtilsLinux` now works in two modes:

1. **`org.freedesktop.resolve1` is available** (systemd-resolved) — the
   existing D-Bus path is used unchanged.
2. **it is not available** — the daemon:
   - uses `resolvconf` (openresolv) when installed:
     `resolvconf -a amneziavpn -x -m 0`;
   - otherwise writes `/etc/resolv.conf` directly, saving the previous file
     and its type (regular file or symlink) in `/var/lib/amneziavpn/`;
   - restores the original configuration on disconnect or `SIGTERM`.

No Python dependencies, no D-Bus name spoofing, no extra daemon.

## What the installer does

1. Locates `AmneziaVPN-service` in `/opt/AmneziaVPN` (or `--app-dir`).
2. Detects the running init system and installs a native service with
   automatic restart:
   - **OpenRC** — `/etc/init.d/amneziavpn` (`supervise-daemon`)
   - **runit** — `/etc/sv/amneziavpn` (`runsv` + `svlogd`)
   - **s6** — `/etc/s6/sv/amneziavpn`
   - **dinit** — `/etc/dinit.d/amneziavpn`
   - **SysVinit / Slackware** — `/etc/init.d/amneziavpn` with a small watchdog
   - **systemd** — unit with `Restart=always` (handled by the existing
     `post_install.sh` path)
3. Purges `iptables`/`ip6tables` jumps and `amnvpn.*` chains on stop.
4. Installs the `/usr/local/bin/AmneziaVPN` launcher, which falls back to
   XWayland when the bundled Qt has no Wayland plugin.
5. Removes the old `/etc/systemd/system/AmneziaVPN.service` when it would
   conflict.

## Automatic installation (official `.run`)

`post_install.sh` detects whether systemd is the running init system. On
systemd it keeps the existing unit path; otherwise it calls
`install.sh --app-dir /opt/AmneziaVPN` from the package. `post_uninstall.sh`
mirrors this by calling `uninstall.sh`. The package ships `install.sh`,
`init/`, `lib/` and `docs/` (see `cmake/CPack.cmake`).

The scripts require `bash`; on Alpine install it first with `apk add bash`.

## Manual installation

For an existing installation or a self-built tree:

```bash
sudo ./install.sh
```

`install.sh` targets alternative init systems. On systemd the package's
`post_install.sh` installs the existing unit instead.

Options:

```bash
sudo ./install.sh --force-x11   # always use XWayland
sudo ./install.sh --no-start    # install services without starting them
sudo ./install.sh --check       # show what would be done
```

Uninstall:

```bash
sudo ./uninstall.sh          # remove services, keep /opt/AmneziaVPN
sudo ./uninstall.sh --purge  # remove everything
```

## Requirements

- `iproute2` (`ip`) and `iptables` or `nftables`;
- `bash`;
- a client build that contains this native patch — the old 5.0.x binary
  still has the D-Bus-only DNS code;
- optional: `openresolv`/`resolvconf` for the preferred DNS backend.

## Verification

```bash
# OpenRC
sudo rc-service amneziavpn status
sudo tail -f /var/log/AmneziaVPN/amneziavpn.log

# runit
sudo sv status amneziavpn
sudo tail -f /var/log/AmneziaVPN/runit-amneziavpn/current

# after connecting
cat /etc/resolv.conf
dig +short example.com

# after disconnecting
cat /etc/resolv.conf

# no leftover firewall chains
sudo iptables -S | grep amnvpn
```

## Repository layout

```
install.sh / uninstall.sh
lib/common.sh                 init detection, binary discovery, templates
init/<init>/service.template  native service templates
docs/                         wayland, dns, s6, slackware
```

All of the above are shipped in the official `.run` by `cmake/CPack.cmake`.

## Testing

Service lifecycle tested on real containers with the official binary:

- **Void Linux / runit** — install, socket creation, `kill -9` respawn,
  uninstall, no leftover processes;
- **Artix Linux / OpenRC** — the same, plus `rc-update` autostart, simulated
  reboot, and `iptables`/`ip6tables` cleanup across `filter`, `nat`, `mangle`
  and `raw`;
- **systemd sandbox** — the existing unit path is unchanged.

The native DNS fallback was compile-checked against Qt 6.12
(`Qt6Core`, `Qt6DBus`, `Qt6Network`). A full tunnel test requires a build
with this patch; the service and firewall behaviour is independent of it.

Real reboot persistence requires an installed system. In a live ISO, verify
the enablement artefact instead — for OpenRC:
`ls -l /etc/runlevels/default/amneziavpn` and
`rc-update show default | grep amneziavpn`.

## License

GPL-3.0-or-later, matching the AmneziaVPN license.
