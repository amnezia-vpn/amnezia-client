# shellcheck shell=bash
# shellcheck disable=SC2034
APP_NAME="AmneziaVPN"
DEFAULT_APP_DIR="/opt/AmneziaVPN"
STATE_DIR="/var/lib/amneziavpn"
LOG_DIR="/var/log/AmneziaVPN"
SERVICE_NAME="amneziavpn"

log()  { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[!]\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31m[x]\033[0m %s\n' "$*" >&2; exit 1; }

need_root() {
    [ "$(id -u)" -eq 0 ] || die "run as root: sudo $0 $*"
}

detect_init() {
    if [ -d /run/systemd/system ] && command -v systemctl >/dev/null 2>&1; then
        echo systemd
    elif [ -f /etc/slackware-version ]; then
        echo slackware
    elif command -v rc-update >/dev/null 2>&1 && command -v rc-service >/dev/null 2>&1; then
        echo openrc
    elif command -v sv >/dev/null 2>&1 && [ -d /etc/sv ]; then
        echo runit
    elif command -v dinitctl >/dev/null 2>&1; then
        echo dinit
    elif [ -d /etc/s6 ] || command -v s6-rc >/dev/null 2>&1; then
        echo s6
    elif [ -d /etc/init.d ]; then
        echo sysvinit
    else
        echo unknown
    fi
}

runit_service_dir() {
    local d
    for d in /var/service /etc/service /run/runit/service; do
        [ -d "$d" ] && { echo "$d"; return; }
    done
    echo /var/service
}

find_service_binary() {
    local dir="$1" p
    for p in \
        "$dir/bin/AmneziaVPN-service" \
        "$dir/AmneziaVPN-service" \
        "$dir/service/bin/AmneziaVPN-service" \
        "$dir/service/AmneziaVPN-service" \
        "$dir/client/bin/AmneziaVPN-service"; do
        [ -x "$p" ] && { printf '%s\n' "$p"; return 0; }
    done
    p="$(find "$dir" -maxdepth 4 -type f -name 'AmneziaVPN-service' -perm -u+x -print -quit 2>/dev/null)"
    [ -n "$p" ] && { printf '%s\n' "$p"; return 0; }
    return 1
}

find_client_binary() {
    local dir="$1" p
    for p in \
        "$dir/bin/AmneziaVPN" \
        "$dir/AmneziaVPN" \
        "$dir/client/bin/AmneziaVPN"; do
        [ -x "$p" ] && { printf '%s\n' "$p"; return 0; }
    done
    p="$(find "$dir" -maxdepth 4 -type f -name 'AmneziaVPN' -perm -u+x -print -quit 2>/dev/null)"
    [ -n "$p" ] && { printf '%s\n' "$p"; return 0; }
    return 1
}

_sed_escape() {
    printf '%s' "$1" | sed -e 's/[\\&|]/\\&/g'
}

render_template() {
    local tpl="$1" out="$2"; shift 2
    local expr=() kv
    for kv in "$@"; do
        expr+=(-e "s|@${kv%%=*}@|$(_sed_escape "${kv#*=}")|g")
    done
    sed "${expr[@]}" "$tpl" > "$out"
}

has_wayland_plugin() {
    local dir="$1" f
    for f in \
        "$dir"/plugins/platforms/libqwayland* \
        "$dir"/lib/plugins/platforms/libqwayland* \
        "$dir"/plugins/libqwayland*; do
        [ -e "$f" ] && return 0
    done
    return 1
}
