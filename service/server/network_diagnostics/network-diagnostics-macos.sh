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

have() { command -v "$1" >/dev/null 2>&1; }

# Runs block function $2, writes a timestamped header plus its output
# (stdout+stderr) to $OUT/$1.txt. Errors inside a block don't abort the script.
save() {
    local name="$1" fn="$2" path
    path="$OUT/$name.txt"
    {
        echo "=== $name === $(date -Iseconds 2>/dev/null || date)"
        "$fn" 2>&1
    } > "$path"
    echo "  [ok] $name"
}

echo "Snapshot '$LABEL' -> $OUT"
[ "$(id -u)" -eq 0 ] || echo "  [warn] not running as root — some data will be incomplete"

# --- 1. System ---------------------------------------------------------
system_block() {
    echo "--- sw_vers ---"
    sw_vers 2>&1
    echo
    echo "--- uname / hostname ---"
    uname -a
    hostname
    echo
    echo "IsRoot: $([ "$(id -u)" -eq 0 ] && echo True || echo False)"
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

# --- 5. Routes ---------------------------------------------------------
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
}
save "routes" routes_block

# --- 6. Actual routing stack resolution --------------------------------------
# What you can't see in `netstat -nr`: where the packet will ACTUALLY go.
route_resolution_block() {
    targets=("8.8.8.8" "1.1.1.1")
    if [ -n "$ENDPOINT" ]; then targets=("$ENDPOINT" "${targets[@]}"); fi
    for t in "${targets[@]}"; do
        echo "--- route -n get $t ---"
        route -n get "$t" 2>&1
        echo
    done
}
save "route-resolution" route_resolution_block

# --- 7. DNS -----------------------------------------------------------------
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
services_processes_block() {
    local pattern='amnezia|wireguard|wg-quick|openvpn|tap|mullvad|tailscale|zapret|nfqws|winws|xray|v2ray|clash|outline|hiddify|proton|nord|express'

    echo "--- launchctl list matching pattern ---"
    have launchctl && launchctl list 2>/dev/null | grep -Ei "$pattern"
    echo
    echo "--- processes ---"
    ps aux | grep -Ei "$pattern" | grep -v grep
    echo
    echo "--- listening TCP sockets (Xray inbounds) ---"
    if have lsof; then
        lsof -nP -iTCP -sTCP:LISTEN 2>&1
    else
        netstat -anp tcp 2>&1
    fi
    echo
    echo "--- UDP endpoints (AmneziaWG) ---"
    if have lsof; then
        lsof -nP -iUDP 2>&1
    else
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
