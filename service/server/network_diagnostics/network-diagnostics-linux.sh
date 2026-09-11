#!/usr/bin/env bash
# amnezia-baseline-snapshot.sh
# Linux network state snapshot for establishing a baseline.
# Run with sudo — otherwise some data (socket owners, iptables,
# some modules) will be incomplete.
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
        echo "=== $name === $(date -Is)  (root=$IS_ROOT)"
        "$fn" 2>&1
    } > "$path"
    echo "  [ok] $name"
}

echo "Snapshot '$LABEL' -> $OUT"
[ "$IS_ROOT" = true ] || echo "  [warn] not running as root — some data will be incomplete"

# --- 1. System ---------------------------------------------------------
system_block() {
    echo "--- OS release ---"
    cat /etc/os-release 2>/dev/null
    echo
    echo "--- Kernel / hostname ---"
    uname -a
    have hostnamectl && hostnamectl 2>&1
    echo
    echo "IsRoot: $IS_ROOT"
    echo "Init system: $(ps -p 1 -o comm= 2>/dev/null)"
}
save "system" system_block

# --- 2. Adapters, including hidden and disabled -----------------------------
# Key for detecting an "overwritten driver" and leftover interfaces.
adapters_block() {
    echo "--- ip -d link (all interfaces, including down) ---"
    ip -d link show
    echo
    echo "--- ip addr ---"
    ip addr show
    echo
    echo "--- driver info (ethtool -i) for each interface ---"
    for p in /sys/class/net/*; do
        ifc=$(basename "$p")
        [ "$ifc" = "lo" ] && continue
        echo "-- $ifc --"
        if have ethtool; then
            ethtool -i "$ifc" 2>&1
        else
            echo "ethtool not installed"
        fi
    done
    echo
    echo "--- nmcli device (if NetworkManager is present) ---"
    have nmcli && nmcli -f all device show 2>&1

    echo
    echo "--- leftover/down interfaces with a UP admin state but no carrier ---"
    echo "(sign of a stuck virtual interface left behind by a previous VPN session)"
    for p in /sys/class/net/*; do
        ifc=$(basename "$p")
        [ "$ifc" = "lo" ] && continue
        flags=$(cat "$p/flags" 2>/dev/null)
        operstate=$(cat "$p/operstate" 2>/dev/null)
        carrier=$(cat "$p/carrier" 2>/dev/null)
        # IFF_UP is bit 0x1 in the flags word
        if [ -n "$flags" ] && (( (flags) & 0x1 )) && [ "$carrier" = "0" ]; then
            echo "$ifc: operstate=$operstate carrier=$carrier"
        fi
    done
}
save "adapters" adapters_block

# --- 3. Connection type (cellular / wifi / ethernet) ------------------------
link_type_block() {
    echo "--- type detection via /sys and iw ---"
    for p in /sys/class/net/*; do
        ifc=$(basename "$p")
        [ "$ifc" = "lo" ] && continue
        t="other"
        if [ -d "$p/wireless" ] || { have iw && iw dev "$ifc" info >/dev/null 2>&1; }; then
            t="wifi"
        elif [[ "$ifc" == wwan* || "$ifc" == ww* || "$ifc" == usb* ]]; then
            t="cellular"
        elif [[ "$ifc" == en* || "$ifc" == eth* ]]; then
            t="ethernet"
        elif [[ "$ifc" == wg* || "$ifc" == tun* || "$ifc" == tap* || "$ifc" == awg* ]]; then
            t="vpn-tunnel"
        fi
        operstate=$(cat "$p/operstate" 2>/dev/null)
        carrier=$(cat "$p/carrier" 2>/dev/null)
        echo "$ifc: type=$t operstate=$operstate carrier=$carrier"
    done
    echo
    echo "--- NetworkManager connection profiles ---"
    have nmcli && nmcli connection show 2>&1

    echo
    echo "--- interface carrying the default route (determines the link type) ---"
    dev=$(ip route show default 2>/dev/null | awk '/^default/{for(i=1;i<=NF;i++) if($i=="dev") print $(i+1)}' | head -n1)
    if [ -n "$dev" ]; then
        metric=$(ip route show default dev "$dev" 2>/dev/null | grep -o 'metric [0-9]*' | head -n1)
        echo "default route dev=$dev $metric"
    else
        echo "no default route"
    fi
}
save "link-type" link_type_block

# --- 4. Drivers: kernel modules and network filters --------------------------
# Catches the wireguard module, tun/tap, and netfilter hooks used by
# split-tunnel solutions.
drivers_block() {
    echo "--- lsmod (all loaded modules) ---"
    lsmod
    echo
    echo "--- VPN/netfilter related modules ---"
    lsmod | grep -E 'wireguard|^tun|^tap|nf_tables|ip_tables|ip6_tables|nfnetlink|xt_|nft_'
    echo
    echo "--- modinfo wireguard (if kernel module, not a userspace implementation) ---"
    have modinfo && modinfo wireguard 2>&1
}
save "drivers" drivers_block

# --- 5. Routes + conflict analysis on metric --------------------------------
routes_block() {
    echo "--- ip route (main table) ---"
    ip route show
    echo
    echo "--- ip route show table all ---"
    ip route show table all
    echo
    echo "--- ip -6 route ---"
    ip -6 route show table all
    echo
    echo "--- ip rule / ip -6 rule (policy routing) ---"
    ip rule show
    ip -6 rule show
    echo
    echo "--- default routes only ---"
    ip route show default
    ip -6 route show default
    echo
    echo "--- interfaces: addresses and indexes ---"
    ip -o addr show

    echo
    echo "--- default route conflicts ---"
    echo "Amnezia's norm: the tunnel wins on the LOWER metric (implicit metric 0 also wins)."
    echo "Conflict: multiple default routes with equal metric."
    for fam in "" "-6"; do
        [ -z "$fam" ] && label="IPv4" || label="IPv6"
        echo "-- $label --"
        # ip prints "metric N" only when the metric was set explicitly; a route without
        # it defaults to metric 0, so a missing token is normalized to "0" here.
        metrics=$(ip $fam route show default 2>/dev/null | awk '
            { m = 0; for (i = 1; i <= NF; i++) if ($i == "metric") m = $(i+1); print m }')
        count=$(ip $fam route show default 2>/dev/null | wc -l)
        if [ "$count" -gt 1 ]; then
            dup=$(echo "$metrics" | sort | uniq -d)
            if [ -n "$dup" ]; then
                echo "!!! CONFLICT: multiple $label default routes with equal metric: $dup"
            else
                echo "OK: $count $label default routes with distinct metrics"
            fi
        fi
    done

    echo
    echo "--- private subnet reachability vs tunnel ---"
    echo "If a private subnet route prefers the tunnel, LAN (router, NAS, printer) becomes unreachable."
    ip route show 2>/dev/null | grep -E '^(10\.|172\.(1[6-9]|2[0-9]|3[01])\.|192\.168\.)'
}
save "routes" routes_block

# --- 6. Actual routing stack resolution --------------------------------------
# What you can't see in `ip route show`: where the packet will ACTUALLY go.
# Also checks the route to the endpoint's own next-hop, since that's where a
# routing loop through the tunnel would actually show up.
route_resolution_block() {
    targets=("8.8.8.8" "1.1.1.1")
    if [ -n "$ENDPOINT" ]; then targets=("$ENDPOINT" "${targets[@]}"); fi

    if [ -n "$ENDPOINT" ]; then
        nexthop=$(ip route get "$ENDPOINT" 2>/dev/null | grep -o 'via [0-9a-fA-F:.]*' | awk '{print $2}' | head -n1)
        if [ -n "$nexthop" ]; then targets+=("$nexthop"); fi
    fi

    for t in "${targets[@]}"; do
        echo "--- ip route get $t ---"
        ip route get "$t" 2>&1
        echo
    done

    if [ -n "$ENDPOINT" ]; then
        echo "--- endpoint reachability ---"
        outdev=$(ip route get "$ENDPOINT" 2>/dev/null | grep -o 'dev [^ ]*' | awk '{print $2}' | head -n1)
        if [ -n "$outdev" ]; then
            case "$outdev" in
                wg*|awg*|tun*|tap*|utun*)
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
        echo "--- host route /32 to endpoint (anti-loop guard) ---"
        ip route show "$ENDPOINT"/32 2>&1
    fi
}
save "route-resolution" route_resolution_block

# --- 7. DNS + leak risk assessment -------------------------------------------
dns_block() {
    echo "--- /etc/resolv.conf ---"
    cat /etc/resolv.conf 2>&1
    echo
    echo "--- resolvectl status (systemd-resolved) ---"
    have resolvectl && resolvectl status 2>&1
    echo
    echo "--- resolvectl dns / domain ---"
    have resolvectl && { resolvectl dns 2>&1; resolvectl domain 2>&1; }
    echo
    echo "--- NetworkManager DNS (VPN clients write here via nm) ---"
    have nmcli && nmcli device show 2>&1 | grep -i 'DNS\|GENERAL.DEVICE'
    echo
    echo "--- /etc/nsswitch.conf (hosts line) ---"
    grep '^hosts' /etc/nsswitch.conf 2>&1

    echo
    echo "--- DNS leak risk ---"
    tundev=$(ip route show default 2>/dev/null | awk '/^default/{for(i=1;i<=NF;i++) if($i=="dev") print $(i+1)}' | head -n1)
    echo "Default route goes through dev=$tundev"
    if have resolvectl; then
        withdns=$(resolvectl status 2>/dev/null | awk '/^Link/{ifc=$3} /DNS Servers/{print ifc}')
        echo "Interfaces with DNS configured: $(echo "$withdns" | grep -c .)"
        other=$(echo "$withdns" | grep -v "^${tundev}$" | grep -v '^$')
        if [ -n "$other" ]; then
            echo "!!! LEAK RISK: DNS is configured on more than just the active interface:"
            echo "$other"
        else
            echo "No obvious risk."
        fi
    else
        echo "resolvectl not available — cannot assess per-interface DNS assignment."
    fi
}
save "dns" dns_block

# --- 8. IPv6 and localhost ---------------------------------------------
ipv6_localhost_block() {
    echo "--- IP addresses (all families) ---"
    ip -o addr show
    echo
    echo "--- IPv6 disabled via sysctl? ---"
    sysctl net.ipv6.conf.all.disable_ipv6 net.ipv6.conf.default.disable_ipv6 2>&1
    echo
    echo "--- localhost resolution ---"
    getent hosts localhost
    getent ahosts localhost
    echo
    echo "--- hosts file ---"
    cat /etc/hosts

    echo
    echo "--- IPv6 leak risk ---"
    echo "Sign: a default ::/0 route exists through the tunnel, but the tunnel only has"
    echo "ULA (fc00::/7) or link-local addresses - no real IPv6 connectivity."
    tun6dev=$(ip -6 route show default 2>/dev/null | awk '/^default/{for(i=1;i<=NF;i++) if($i=="dev") print $(i+1)}' | head -n1)
    if [ -n "$tun6dev" ]; then
        echo "::/0 goes through dev=$tun6dev"
        addrs=$(ip -6 -o addr show dev "$tun6dev" 2>/dev/null | awk '{print $4}')
        echo "$addrs"
        global=$(echo "$addrs" | grep -vE '^(fe80|fc|fd)')
        if [ -z "$global" ]; then
            echo "!!! Only ULA/link-local addresses - real IPv6 traffic will leak outside the tunnel."
        fi
    else
        echo "::/0 is absent - no IPv6 routing."
    fi
}
save "ipv6-localhost" ipv6_localhost_block

# --- 9. Proxy: environment and system settings ------------------------------
# Key block for Xray: it often works through the system proxy.
proxy_block() {
    echo "--- environment variables ---"
    env | grep -i proxy
    echo
    echo "--- /etc/environment ---"
    grep -i proxy /etc/environment 2>/dev/null
    echo
    echo "--- GNOME gsettings proxy (if present) ---"
    have gsettings && gsettings get org.gnome.system.proxy mode 2>&1
    echo
    echo "--- apt proxy config (if present) ---"
    cat /etc/apt/apt.conf.d/*proxy* 2>/dev/null
}
save "proxy" proxy_block

# --- 10. VPN/antiDPI services and processes ----------------------------
# Matched by exact process/unit name rather than a broad substring, since a
# substring match also pulls in unrelated software installed on the machine.
services_processes_block() {
    local -a known_units=(
        amnezia-server amneziawg wg-quick@awg0 wg-quick@wg0
        openvpn openvpn-client mullvad-daemon tailscaled
        nfqws winws zapret xray-service v2ray sing-box
        clash outline-server hiddify NordVPN protonvpn
    )
    local -a known_procs=(
        AmneziaVPN amnezia-service wireguard wg-quick openvpn
        mullvad-daemon tailscaled nfqws winws zapret goodbyedpi
        byedpi ciadpi xray v2ray sing-box tun2socks clash-verge
    )

    echo "--- systemd units matching known exact names ---"
    if have systemctl; then
        systemctl list-units --all --type=service --no-legend 2>/dev/null |
            awk -v units="$(IFS='|'; echo "${known_units[*]}")" '
                BEGIN { n = split(units, arr, "|") }
                { for (i = 1; i <= n; i++) if (index($1, arr[i]".service") == 1) { print; break } }'
    fi
    echo
    echo "--- processes matching known exact names ---"
    for p in "${known_procs[@]}"; do
        pgrep -a -x "$p" 2>/dev/null
    done
    echo
    echo "--- listening TCP sockets belonging to those processes ---"
    if have ss; then
        ss -tlnp 2>&1 | grep -E "$(IFS='|'; echo "${known_procs[*]}")" || echo "(none matched)"
    fi
    echo
    echo "--- UDP endpoints belonging to those processes ---"
    if have ss; then
        ss -ulnp 2>&1 | grep -E "$(IFS='|'; echo "${known_procs[*]}")" || echo "(none matched)"
    fi
    echo
    echo "--- full listening socket list (unfiltered, for reference) ---"
    if have ss; then
        ss -tulnp
    else
        netstat -tulnp
    fi
}
save "services-processes" services_processes_block

# --- 11. Firewall / netfilter ------------------------------------------------
firewall_block() {
    echo "--- iptables -L -n -v ---"
    have iptables && iptables -L -n -v 2>&1
    echo
    echo "--- ip6tables -L -n -v ---"
    have ip6tables && ip6tables -L -n -v 2>&1
    echo
    echo "--- nft list ruleset (first ~400 lines) ---"
    have nft && nft list ruleset 2>&1 | head -n 400
    echo "(output truncated; see netfilter-full.txt for the full dump)"
    echo
    echo "--- ufw status ---"
    have ufw && ufw status verbose 2>&1
    echo
    echo "--- firewalld ---"
    if have firewall-cmd; then
        firewall-cmd --state 2>&1
        firewall-cmd --list-all 2>&1
    fi
}
save "firewall" firewall_block

# --- 12. Full netfilter ruleset dump (heavy output, separate file) ----------
netfilter_full_block() {
    echo "--- iptables-save ---"
    have iptables-save && iptables-save 2>&1
    echo
    echo "--- ip6tables-save ---"
    have ip6tables-save && ip6tables-save 2>&1
    echo
    echo "--- nft list ruleset (full) ---"
    have nft && nft list ruleset 2>&1
}
save "netfilter-full" netfilter_full_block

echo
echo "Done. Folder: $OUT"
echo "Diff of two runs, e.g.:"
echo "  diff snapshot-clean/routes.txt snapshot-awg/routes.txt"
