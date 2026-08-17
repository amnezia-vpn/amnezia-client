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

have() { command -v "$1" >/dev/null 2>&1; }

# Runs block function $2, writes a timestamped header plus its output
# (stdout+stderr) to $OUT/$1.txt. Errors inside a block don't abort the script.
save() {
    local name="$1" fn="$2" path
    path="$OUT/$name.txt"
    {
        echo "=== $name === $(date -Is)"
        "$fn" 2>&1
    } > "$path"
    echo "  [ok] $name"
}

echo "Snapshot '$LABEL' -> $OUT"
[ "$(id -u)" -eq 0 ] || echo "  [warn] not running as root — some data will be incomplete"

# --- 1. System ---------------------------------------------------------
system_block() {
    echo "--- OS release ---"
    cat /etc/os-release 2>/dev/null
    echo
    echo "--- Kernel / hostname ---"
    uname -a
    have hostnamectl && hostnamectl 2>&1
    echo
    echo "IsRoot: $([ "$(id -u)" -eq 0 ] && echo True || echo False)"
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
        elif [[ "$ifc" == wg* || "$ifc" == tun* || "$ifc" == tap* ]]; then
            t="vpn-tunnel"
        fi
        operstate=$(cat "$p/operstate" 2>/dev/null)
        carrier=$(cat "$p/carrier" 2>/dev/null)
        echo "$ifc: type=$t operstate=$operstate carrier=$carrier"
    done
    echo
    echo "--- NetworkManager connection profiles ---"
    have nmcli && nmcli connection show 2>&1
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

# --- 5. Routes ---------------------------------------------------------
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
}
save "routes" routes_block

# --- 6. Actual routing stack resolution --------------------------------------
# What you can't see in `ip route show`: where the packet will ACTUALLY go.
route_resolution_block() {
    targets=("8.8.8.8" "1.1.1.1")
    if [ -n "$ENDPOINT" ]; then targets=("$ENDPOINT" "${targets[@]}"); fi
    for t in "${targets[@]}"; do
        echo "--- ip route get $t ---"
        ip route get "$t" 2>&1
        echo
    done
}
save "route-resolution" route_resolution_block

# --- 7. DNS -----------------------------------------------------------------
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
services_processes_block() {
    local pattern='amnezia|wireguard|wg-quick|openvpn|tap|mullvad|tailscale|zapret|nfqws|winws|xray|v2ray|clash|outline|hiddify|proton|nord|express'

    echo "--- systemd units matching pattern ---"
    if have systemctl; then
        systemctl list-units --all --type=service --no-legend 2>/dev/null | grep -Ei "$pattern"
    fi
    echo
    echo "--- processes ---"
    ps aux | grep -Ei "$pattern" | grep -v grep
    echo
    echo "--- listening TCP sockets (Xray inbounds) ---"
    if have ss; then
        ss -tlnp
    else
        netstat -tlnp
    fi
    echo
    echo "--- UDP endpoints (AmneziaWG) ---"
    if have ss; then
        ss -ulnp
    else
        netstat -ulnp
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
