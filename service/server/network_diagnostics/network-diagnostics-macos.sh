#!/usr/bin/env bash
# amnezia-baseline-snapshot.sh
# macOS network state snapshot for establishing a baseline.
# Run with sudo — otherwise some data (socket owners, pf rules,
# some kext/system_profiler details) will be incomplete.
#
# Usage (three runs):
#   sudo ./amnezia-baseline-snapshot.sh --label clean
#   sudo ./amnezia-baseline-snapshot.sh --label awg   --endpoint <server IP>
#   sudo ./amnezia-baseline-snapshot.sh --label xray  --endpoint <server IP>
#
# Then compare the folders, e.g.:
#   diff snapshot-clean/routes.txt snapshot-awg/routes.txt

set -uo pipefail

LABEL="snapshot"
ENDPOINT=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --label)      LABEL="$2"; shift 2 ;;
        --label=*)    LABEL="${1#*=}"; shift ;;
        --endpoint)   ENDPOINT="$2"; shift 2 ;;
        --endpoint=*) ENDPOINT="${1#*=}"; shift ;;
        -h|--help)
            grep '^#' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "Unknown argument: $1" >&2
            exit 1
            ;;
    esac
done

OUT="$(pwd)/snapshot-${LABEL}"
mkdir -p "$OUT"

IS_ROOT=$([ "$(id -u)" -eq 0 ] && echo true || echo false)

have() { command -v "$1" >/dev/null 2>&1; }

# Runs block function $2, writes a timestamped header plus its output
# (stdout+stderr) to $OUT/$1.txt. Errors inside a block don't abort the script.
save() {
    local name="$1" fn="$2" path
    path="$OUT/$name.txt"
    {
        echo "=== $name === $(date -Iseconds 2>/dev/null || date)  (root=$IS_ROOT)"
        "$fn" 2>&1
    } > "$path"
    echo "  [ok] $name"
}

echo "Snapshot '$LABEL' -> $OUT"
[ "$IS_ROOT" = true ] || echo "  [warn] not running as root — some data will be incomplete"

# --- 1. System ---------------------------------------------------------
system_block() {
    echo "--- sw_vers ---"
    sw_vers 2>&1
    echo
    echo "--- uname / hostname ---"
    uname -a
    hostname
    echo
    echo "IsRoot: $IS_ROOT"
}
save "system" system_block

# --- 2. Adapters, including hidden and disabled -----------------------------
# Key for detecting an "overwritten driver" and leftover interfaces.
adapters_block() {
    echo "--- ifconfig -a (all interfaces, including down) ---"
    ifconfig -a
    echo
    echo "--- networksetup -listallhardwareports ---"
    have networksetup && networksetup -listallhardwareports 2>&1
    echo
    echo "--- networksetup -listallnetworkservices ---"
    have networksetup && networksetup -listallnetworkservices 2>&1

    echo
    echo "--- leftover tunnel-like interfaces with no active traffic ---"
    echo "(utun/awg/wg present but status is not 'active', sign of a stuck previous session)"
    for ifc in $(ifconfig -l 2>/dev/null); do
        case "$ifc" in
            utun*|awg*|wg*)
                status=$(ifconfig "$ifc" 2>/dev/null | awk '/status:/{print $2}')
                if [ -n "$status" ] && [ "$status" != "active" ]; then
                    echo "$ifc: status=$status"
                fi
                ;;
        esac
    done
}
save "adapters" adapters_block

# --- 3. Connection type (cellular / wifi / ethernet) ------------------------
link_type_block() {
    echo "--- type detection via ifconfig media / networksetup ---"
    for ifc in $(ifconfig -l 2>/dev/null); do
        [ "$ifc" = "lo0" ] && continue
        t="other"
        case "$ifc" in
            en*)
                if have networksetup && networksetup -getairportnetwork "$ifc" >/dev/null 2>&1; then
                    t="wifi"
                else
                    t="ethernet-or-wifi"
                fi
                ;;
            pdp_ip*|wwan*)   t="cellular" ;;
            utun*|ppp*|ipsec*) t="vpn-tunnel" ;;
            awg*|wg*)        t="vpn-tunnel" ;;
        esac
        status=$(ifconfig "$ifc" 2>/dev/null | awk '/status:/{print $2}')
        echo "$ifc: type=$t status=$status"
    done
    echo
    echo "--- current Wi-Fi network (if any) ---"
    if have networksetup; then
        for ifc in $(ifconfig -l 2>/dev/null); do
            case "$ifc" in en*)
                networksetup -getairportnetwork "$ifc" 2>&1
            ;; esac
        done
    fi
    echo
    echo "--- scutil --nwi (primary interface / reachability) ---"
    have scutil && scutil --nwi 2>&1

    echo
    echo "--- interface carrying the default route (determines the link type) ---"
    dev=$(route -n get default 2>/dev/null | awk '/interface:/{print $2}')
    if [ -n "$dev" ]; then
        echo "default route dev=$dev"
    else
        echo "no default route"
    fi
}
save "link-type" link_type_block

# --- 4. Drivers: kernel extensions and system network config ----------------
# Catches the WireGuard/tun kext or sysex, and split-tunnel related extensions.
drivers_block() {
    echo "--- kextstat (legacy kexts, if any) ---"
    have kextstat && kextstat 2>&1
    echo
    echo "--- systemextensionsctl list (modern DriverKit/NetworkExtension sysexes) ---"
    have systemextensionsctl && systemextensionsctl list 2>&1
    echo
    echo "--- VPN/netfilter related kexts ---"
    have kextstat && kextstat 2>&1 | grep -iE 'wireguard|tun|tap|utun|amnezia'
}
save "drivers" drivers_block

# --- 5. Routes + conflict analysis -------------------------------------
routes_block() {
    echo "--- netstat -nr (IPv4 + IPv6 routing table) ---"
    netstat -nr
    echo
    echo "--- netstat -nr -f inet (IPv4 only) ---"
    netstat -nr -f inet
    echo
    echo "--- netstat -nr -f inet6 (IPv6 only) ---"
    netstat -nr -f inet6
    echo
    echo "--- default routes only ---"
    netstat -nr | awk 'NR==1 || /^default/'
    echo
    echo "--- interfaces: addresses ---"
    ifconfig -a | grep -E '^[a-z]|inet '

    echo
    echo "--- default route conflicts ---"
    echo "macOS auto-creates a link-local (fe80::%ifc) IPv6 'default' entry per utun"
    echo "interface regardless of VPN state — that's normal noise, not a conflict."
    echo "Amnezia's AWG tunnel also often uses a split route (::/1 + 8000::/1) instead"
    echo "of a literal ::/0 default, so only non-link-local default gateways are counted."
    for fam in inet inet6; do
        [ "$fam" = "inet" ] && label="IPv4" || label="IPv6"
        count=$(netstat -nr -f "$fam" 2>/dev/null | awk '/^default/ && $2 !~ /^fe80:/' | wc -l | tr -d ' ')
        echo "-- $label: $count non-link-local default route(s) --"
        if [ "$count" -gt 1 ]; then
            echo "!!! CONFLICT: multiple $label default routes present"
            netstat -nr -f "$fam" 2>/dev/null | awk 'NR==1 || (/^default/ && $2 !~ /^fe80:/)'
        fi
    done

    echo
    echo "--- private subnet reachability vs tunnel ---"
    echo "If a private subnet route prefers the tunnel, LAN (router, NAS, printer) becomes unreachable."
    netstat -nr -f inet 2>/dev/null | grep -E '^(10\.|172\.(1[6-9]|2[0-9]|3[01])\.|192\.168\.)'
}
save "routes" routes_block

# --- 6. Actual routing stack resolution --------------------------------------
# What you can't see in `netstat -nr`: where the packet will ACTUALLY go.
# Also checks the route to the endpoint's own next-hop, since that's where a
# routing loop through the tunnel would actually show up.
route_resolution_block() {
    targets=("8.8.8.8" "1.1.1.1")
    if [ -n "$ENDPOINT" ]; then targets=("$ENDPOINT" "${targets[@]}"); fi

    if [ -n "$ENDPOINT" ]; then
        nexthop=$(route -n get "$ENDPOINT" 2>/dev/null | awk '/gateway:/{print $2}')
        if [ -n "$nexthop" ]; then targets+=("$nexthop"); fi
    fi

    for t in "${targets[@]}"; do
        echo "--- route -n get $t ---"
        route -n get "$t" 2>&1
        echo
    done

    if [ -n "$ENDPOINT" ]; then
        echo "--- endpoint reachability ---"
        outdev=$(route -n get "$ENDPOINT" 2>/dev/null | awk '/interface:/{print $2}')
        if [ -n "$outdev" ]; then
            case "$outdev" in
                utun*|awg*|wg*|ppp*|ipsec*)
                    echo "!!! LOOP: traffic to server $ENDPOINT goes through the VIRTUAL/TUNNEL interface '$outdev'"
                    ;;
                *)
                    echo "OK: traffic to server goes through the physical interface '$outdev'"
                    ;;
            esac
        else
            echo "Could not determine the interface."
        fi

        echo
        echo "--- host route to endpoint (anti-loop guard) ---"
        route -n get -host "$ENDPOINT" 2>&1
    fi
}
save "route-resolution" route_resolution_block

# --- 7. DNS + leak risk assessment -------------------------------------------
dns_block() {
    echo "--- scutil --dns (effective per-interface resolver config) ---"
    have scutil && scutil --dns 2>&1
    echo
    echo "--- /etc/resolv.conf (usually managed/symlinked on macOS) ---"
    cat /etc/resolv.conf 2>&1
    echo
    echo "--- networksetup -getdnsservers per network service ---"
    if have networksetup; then
        while IFS= read -r svc; do
            [[ "$svc" == "An asterisk"* ]] && continue
            echo "-- $svc --"
            networksetup -getdnsservers "$svc" 2>&1
        done < <(networksetup -listallnetworkservices 2>/dev/null | tail -n +2)
    fi

    echo
    echo "--- DNS leak risk ---"
    tundev=$(route -n get default 2>/dev/null | awk '/interface:/{print $2}')
    echo "Default route goes through dev=$tundev"
    if have scutil; then
        # Each "resolver #N" block may carry an "if_index : N (ifc)" line and one or
        # more "nameserver[N] : ..." lines, in either order; scan a whole block before
        # deciding whether it counted as "has DNS on interface X".
        withdns=$(scutil --dns 2>/dev/null | awk '
            /^resolver #/ { ifc=""; hasdns=0 }
            /if_index/ { line=$0; sub(/.*\(/, "", line); sub(/\).*/, "", line); ifc=line }
            /nameserver\[[0-9]+\]/ { hasdns=1 }
            /^$/ { if (ifc != "" && hasdns) print ifc }
            END { if (ifc != "" && hasdns) print ifc }')
        echo "Interfaces with DNS configured: $(echo "$withdns" | sort -u | grep -c .)"
        other=$(echo "$withdns" | sort -u | grep -v "^${tundev}$" | grep -v '^$')
        if [ -n "$other" ]; then
            echo "!!! LEAK RISK: DNS is configured on more than just the active interface:"
            echo "$other"
        else
            echo "No obvious risk."
        fi
    fi
}
save "dns" dns_block

# --- 8. IPv6 and localhost ---------------------------------------------
ipv6_localhost_block() {
    echo "--- IP addresses (all families) ---"
    ifconfig -a | grep -E '^[a-z]|inet '
    echo
    echo "--- IPv6 disabled? (per-interface sysctl / networksetup) ---"
    sysctl net.inet6.ip6.forwarding 2>&1
    if have networksetup; then
        while IFS= read -r svc; do
            [[ "$svc" == "An asterisk"* ]] && continue
            echo "-- $svc --"
            networksetup -getinfo "$svc" 2>&1 | grep -i ipv6
        done < <(networksetup -listallnetworkservices 2>/dev/null | tail -n +2)
    fi
    echo
    echo "--- localhost resolution ---"
    dscacheutil -q host -a name localhost 2>&1
    echo
    echo "--- hosts file ---"
    cat /etc/hosts

    echo
    echo "--- IPv6 leak risk ---"
    echo "Sign: a default IPv6 route exists through the tunnel, but the tunnel only has"
    echo "ULA (fc00::/7) or link-local addresses - no real IPv6 connectivity."
    # macOS auto-creates a link-local (fe80::%ifc) 'default' entry per utun interface
    # regardless of VPN state, and a split-tunnel VPN may install ::/1 instead of a
    # literal ::/0 default — so skip link-local noise and accept either form.
    tun6dev=$(netstat -nr -f inet6 2>/dev/null | awk '(/^default/ || /^::\/1/) && $2 !~ /^fe80:/{print $NF; exit}')
    if [ -n "$tun6dev" ]; then
        echo "IPv6 default route goes through dev=$tun6dev"
        addrs=$(ifconfig "$tun6dev" 2>/dev/null | awk '/inet6/{print $2}')
        echo "$addrs"
        global=$(echo "$addrs" | grep -vE '^(fe80|fc|fd)')
        if [ -z "$global" ]; then
            echo "!!! Only ULA/link-local addresses - real IPv6 traffic will leak outside the tunnel."
        fi
    else
        echo "No IPv6 default route - no IPv6 routing."
    fi
}
save "ipv6-localhost" ipv6_localhost_block

# --- 9. Proxy: environment and system settings ------------------------------
# Key block for Xray: it often works through the system proxy.
proxy_block() {
    echo "--- environment variables ---"
    env | grep -i proxy
    echo
    echo "--- scutil --proxy (system-wide proxy configuration) ---"
    have scutil && scutil --proxy 2>&1
    echo
    echo "--- networksetup proxy settings per network service ---"
    if have networksetup; then
        while IFS= read -r svc; do
            [[ "$svc" == "An asterisk"* ]] && continue
            echo "-- $svc --"
            networksetup -getwebproxy "$svc" 2>&1
            networksetup -getsecurewebproxy "$svc" 2>&1
            networksetup -getsocksfirewallproxy "$svc" 2>&1
        done < <(networksetup -listallnetworkservices 2>/dev/null | tail -n +2)
    fi
}
save "proxy" proxy_block

# --- 10. VPN/antiDPI services and processes ----------------------------
# Matched by exact process/label name rather than a broad substring, since a
# substring match also pulls in unrelated software installed on the machine.
services_processes_block() {
    local -a known_labels=(
        org.amnezia.vpn.service com.wireguard.macos
        net.openvpn.client mullvad-daemon
        com.tailscale.ipn io.tailscale.ipn.macsys
        com.nordvpn.osx ch.protonvpn.mac
    )
    local -a known_procs=(
        AmneziaVPN amnezia-service wireguard-go wg-quick openvpn
        mullvad-daemon tailscaled nfqws winws zapret goodbyedpi
        byedpi ciadpi xray v2ray sing-box tun2socks
    )

    echo "--- launchctl entries matching known exact labels ---"
    if have launchctl; then
        for label in "${known_labels[@]}"; do
            launchctl list 2>/dev/null | awk -v l="$label" '$3==l'
        done
    fi
    echo
    echo "--- processes matching known exact names ---"
    for p in "${known_procs[@]}"; do
        pgrep -l -x "$p" 2>/dev/null
    done
    echo
    echo "--- listening TCP sockets belonging to those processes ---"
    if have lsof; then
        for p in "${known_procs[@]}"; do
            lsof -nP -iTCP -sTCP:LISTEN -c "$p" 2>/dev/null
        done
    fi
    echo
    echo "--- UDP endpoints belonging to those processes ---"
    if have lsof; then
        for p in "${known_procs[@]}"; do
            lsof -nP -iUDP -c "$p" 2>/dev/null
        done
    fi
    echo
    echo "--- full listening socket list (unfiltered, for reference) ---"
    if have lsof; then
        lsof -nP -iTCP -sTCP:LISTEN 2>&1
        lsof -nP -iUDP 2>&1
    else
        netstat -anp tcp 2>&1
        netstat -anp udp 2>&1
    fi
}
save "services-processes" services_processes_block

# --- 11. Firewall / packet filter ------------------------------------------------
# Amnezia loads its rules under the pf anchor "amn" (see macosfirewall.cpp).
firewall_block() {
    echo "--- pfctl -s info (pf enabled/disabled, stats) ---"
    have pfctl && pfctl -s info 2>&1
    echo
    echo "--- pfctl -s Anchors (all loaded anchors) ---"
    have pfctl && pfctl -s Anchors 2>&1
    echo
    echo "--- pfctl -a amn -s rules (Amnezia's own anchor + sub-anchors) ---"
    if have pfctl; then
        anchors=$(pfctl -s Anchors 2>/dev/null | awk '/^amn/ {sub(/\*$/, "", $1); print $1}')
        for anc in $anchors; do
            echo "-- anchor: $anc --"
            pfctl -a "$anc" -s rules 2>&1
        done
    fi
    echo
    echo "--- pfctl -s rules (rules in the main ruleset, not under any anchor) ---"
    have pfctl && pfctl -s rules 2>&1
    echo
    echo "--- socketfilterfw (Application Firewall) ---"
    if [ -x /usr/libexec/ApplicationFirewall/socketfilterfw ]; then
        /usr/libexec/ApplicationFirewall/socketfilterfw --getglobalstate 2>&1
        /usr/libexec/ApplicationFirewall/socketfilterfw --listapps 2>&1
    fi
}
save "firewall" firewall_block

echo
echo "Done. Folder: $OUT"
echo "Diff of two runs, e.g.:"
echo "  diff snapshot-clean/routes.txt snapshot-awg/routes.txt"
