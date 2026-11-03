#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/lib/common.sh"

APP_DIR="$DEFAULT_APP_DIR"
PURGE=0

usage() {
    cat <<EOF
Usage: sudo $0 [options]

  --app-dir DIR   path to the AmneziaVPN installation (default: $DEFAULT_APP_DIR)
  --purge         remove $APP_DIR, logs and state as well
  -h, --help      show this help
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --app-dir) APP_DIR="${2:?}"; shift 2 ;;
        --purge)   PURGE=1; shift ;;
        -h|--help) usage; exit 0 ;;
        *)         die "unknown argument: $1 (see --help)" ;;
    esac
done

need_root "$@"
INIT="$(detect_init)"
if [ "$INIT" = "systemd" ]; then
    die "systemd is handled by post_uninstall.sh; use the package uninstaller"
fi
log "init system: $INIT"

remove_service() {
    local name="$1"
    case "$INIT" in
    openrc)
        rc-service "$name" stop >/dev/null 2>&1 || true
        rc-update del "$name" default >/dev/null 2>&1 || true
        rm -f "/etc/init.d/${name}"
        ;;
    runit)
        svdir="$(runit_service_dir)"
        SVDIR="$svdir" sv down "$name" >/dev/null 2>&1 || true
        rm -f "$svdir/${name}"
        i=0
        while [ "$i" -lt 5 ]; do
            if rm -rf "/etc/sv/${name}" 2>/dev/null; then
                break
            fi
            sleep 1
            i=$((i + 1))
        done
        ;;
    s6)
        rm -rf "/etc/s6/sv/${name}"
        rm -f "/etc/s6/adminsv/default/contents.d/${name}"
        if command -v s6-rc-compile >/dev/null 2>&1 && [ -d /etc/s6/rc ]; then
            s6-rc-compile /etc/s6/rc/compiled /etc/s6/sv >/dev/null 2>&1 || true
            s6-rc-update /etc/s6/rc/compiled >/dev/null 2>&1 || true
        fi
        ;;
    dinit)
        dinitctl stop "$name" >/dev/null 2>&1 || true
        dinitctl disable "$name" >/dev/null 2>&1 || true
        rm -f "/etc/dinit.d/${name}"
        ;;
    sysvinit)
        "/etc/init.d/${name}" stop >/dev/null 2>&1 || true
        update-rc.d -f "$name" remove >/dev/null 2>&1 || true
        rm -f "/etc/init.d/${name}"
        ;;
    slackware)
        "/etc/rc.d/rc.${name}" stop >/dev/null 2>&1 || true
        rm -f "/etc/rc.d/rc.${name}"
        if [ -f /etc/rc.d/rc.local ]; then
            sed -i "\|rc\.${name} start|d" /etc/rc.d/rc.local || true
        fi
        ;;
    esac
}

restore_resolv_conf() {
    if command -v resolvconf >/dev/null 2>&1; then
        resolvconf -d amneziavpn >/dev/null 2>&1 || true
    fi

    [ -f "$STATE_DIR/resolv.conf.meta" ] || return 0
    local kind
    kind="$(cat "$STATE_DIR/resolv.conf.meta")"

    case "$kind" in
        symlink:*)
            local target="${kind#symlink:}"
            rm -f /etc/resolv.conf
            ln -s "$target" /etc/resolv.conf
            ;;
        *)
            [ -f "$STATE_DIR/resolv.conf.bak" ] && cp -f "$STATE_DIR/resolv.conf.bak" /etc/resolv.conf
            ;;
    esac
    rm -f "$STATE_DIR/resolv.conf.meta" "$STATE_DIR/resolv.conf.bak"
    log "restored /etc/resolv.conf"
}

log "stopping services"
remove_service "$SERVICE_NAME"

if [ -f /etc/systemd/system/AmneziaVPN.service ]; then
    rm -f /etc/systemd/system/AmneziaVPN.service
fi

pkill -f 'bin/AmneziaVPN-service' 2>/dev/null || true
pkill -f 'AmneziaVPN-service$' 2>/dev/null || true

restore_resolv_conf

log "removing files"
rm -f /usr/local/bin/AmneziaVPN
rm -f /usr/share/applications/AmneziaVPN.desktop
rm -f /usr/share/pixmaps/AmneziaVPN.png
rm -f /usr/share/icons/hicolor/512x512/apps/AmneziaVPN.png
rm -f /usr/share/icons/hicolor/scalable/apps/AmneziaVPN.svg

if [ "$PURGE" = 1 ]; then
    log "purging $APP_DIR and logs"
    rm -rf "$APP_DIR" "$LOG_DIR" "$STATE_DIR"
else
    rm -rf "$STATE_DIR"
    log "kept $APP_DIR and $LOG_DIR (use --purge to remove them)"
fi

log "done"
