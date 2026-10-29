#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/lib/common.sh"

APP_DIR="$DEFAULT_APP_DIR"
FORCE_X11="auto"
DO_START=1
CHECK_ONLY=0

usage() {
    cat <<EOF
Usage: sudo $0 [options]

  --app-dir DIR     path to the AmneziaVPN installation (default: $DEFAULT_APP_DIR)
  --force-x11       always use XWayland (QT_QPA_PLATFORM=xcb)
  --no-force-x11    leave QT_QPA_PLATFORM unchanged
  --no-start        install services without starting them
  --check           show what would be done, change nothing
  -h, --help        show this help

Examples:
  sudo $0 --app-dir /opt/AmneziaVPN
  sudo $0 --force-x11
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --app-dir)      APP_DIR="${2:?}"; shift 2 ;;
        --force-x11)    FORCE_X11="yes"; shift ;;
        --no-force-x11) FORCE_X11="no"; shift ;;
        --no-start)     DO_START=0; shift ;;
        --check)        CHECK_ONLY=1; shift ;;
        -h|--help)      usage; exit 0 ;;
        *)              die "unknown argument: $1 (see --help)" ;;
    esac
done

SVC_NAME=""
SVC_DESC=""
SVC_BIN=""
SVC_ARGS=""
SVC_OPENRC_DEPS=""
SVC_DINIT_DEPS=""

render_service() {
    local tpl="$1" out="$2"
    render_template "$tpl" "$out" \
        "NAME=$SVC_NAME" \
        "DESC=$SVC_DESC" \
        "BIN=$SVC_BIN" \
        "ARGS=$SVC_ARGS" \
        "LOGDIR=$LOG_DIR" \
        "RUNDIR=/run" \
        "OPENRC_DEPS=$SVC_OPENRC_DEPS" \
        "DINIT_DEPS=$SVC_DINIT_DEPS"
}

install_service() {
    local tmp
    tmp="$(mktemp -d "$WORK_DIR/svc.XXXXXX")"

    case "$INIT" in
    openrc)
        render_service "$SCRIPT_DIR/init/openrc/service.template" "$tmp/initd"
        install -D -m 0755 "$tmp/initd" "/etc/init.d/${SVC_NAME}"
        rc-update add "$SVC_NAME" default >/dev/null 2>&1 \
            || warn "failed to enable $SVC_NAME at boot: rc-update add $SVC_NAME default"
        if [ "$DO_START" = 1 ]; then
            if rc-service "$SVC_NAME" status >/dev/null 2>&1; then
                rc-service "$SVC_NAME" restart \
                    || warn "failed to restart $SVC_NAME: rc-service $SVC_NAME status"
            else
                rc-service "$SVC_NAME" start \
                    || warn "failed to start $SVC_NAME: rc-service $SVC_NAME status"
            fi
        fi
        ;;
    runit)
        install -d -m 0755 "/etc/sv/${SVC_NAME}/log"
        render_service "$SCRIPT_DIR/init/runit/run.template" "$tmp/run"
        render_service "$SCRIPT_DIR/init/runit/finish.template" "$tmp/finish"
        render_service "$SCRIPT_DIR/init/runit/log-run.template" "$tmp/log-run"
        install -m 0755 "$tmp/run" "/etc/sv/${SVC_NAME}/run"
        install -m 0755 "$tmp/finish" "/etc/sv/${SVC_NAME}/finish"
        install -m 0755 "$tmp/log-run" "/etc/sv/${SVC_NAME}/log/run"
        svdir="$(runit_service_dir)"
        if [ -L "$svdir" ] && [ ! -d "$svdir" ]; then
            svtarget="$(readlink "$svdir")"
            case "$svtarget" in
                /*) ;;
                *) svtarget="$(dirname "$svdir")/$svtarget" ;;
            esac
            install -d -m 0755 "$svtarget"
        fi
        install -d -m 0755 "$svdir"
        ln -sfn "/etc/sv/${SVC_NAME}" "$svdir/${SVC_NAME}"
        if [ "$DO_START" = 1 ]; then
            i=0
            while [ "$i" -lt 8 ]; do
                if SVDIR="$svdir" sv restart "$SVC_NAME" >/dev/null 2>&1 \
                    || SVDIR="$svdir" sv up "$SVC_NAME" >/dev/null 2>&1; then
                    break
                fi
                sleep 1
                i=$((i + 1))
            done
            [ "$i" -lt 8 ] || warn "runit has not picked up $SVC_NAME: sv status $SVC_NAME"
        fi
        ;;
    s6)
        install -d -m 0755 "/etc/s6/sv/${SVC_NAME}"
        render_service "$SCRIPT_DIR/init/s6/type.template" "$tmp/type"
        render_service "$SCRIPT_DIR/init/s6/run.template" "$tmp/run"
        render_service "$SCRIPT_DIR/init/s6/finish.template" "$tmp/finish"
        install -m 0644 "$tmp/type" "/etc/s6/sv/${SVC_NAME}/type"
        install -m 0755 "$tmp/run" "/etc/s6/sv/${SVC_NAME}/run"
        install -m 0755 "$tmp/finish" "/etc/s6/sv/${SVC_NAME}/finish"
        if [ -d /etc/s6/adminsv/default/contents.d ]; then
            touch "/etc/s6/adminsv/default/contents.d/${SVC_NAME}"
            if command -v s6-rc-compile >/dev/null 2>&1 && [ -d /etc/s6/rc ]; then
                s6-rc-compile /etc/s6/rc/compiled /etc/s6/sv 2>/dev/null || true
                s6-rc-update /etc/s6/rc/compiled 2>/dev/null || true
            fi
        else
            warn "s6: add ${SVC_NAME} to your bundle manually (see docs/s6.md)"
        fi
        ;;
    dinit)
        render_service "$SCRIPT_DIR/init/dinit/service.template" "$tmp/dinit"
        install -D -m 0644 "$tmp/dinit" "/etc/dinit.d/${SVC_NAME}"
        dinitctl enable "$SVC_NAME" >/dev/null 2>&1 \
            || warn "failed to enable $SVC_NAME at boot: dinitctl enable $SVC_NAME"
        if [ "$DO_START" = 1 ]; then
            dinitctl start "$SVC_NAME" 2>/dev/null || dinitctl restart "$SVC_NAME" 2>/dev/null || true
        fi
        ;;
    sysvinit)
        render_service "$SCRIPT_DIR/init/sysvinit/service.template" "$tmp/initd"
        install -D -m 0755 "$tmp/initd" "/etc/init.d/${SVC_NAME}"
        if command -v update-rc.d >/dev/null 2>&1; then
            update-rc.d "$SVC_NAME" defaults >/dev/null 2>&1 \
                || warn "failed to run: update-rc.d $SVC_NAME defaults"
        else
            for rl in 2 3 4 5; do
                ln -sfn "../init.d/${SVC_NAME}" "/etc/rc${rl}.d/S99${SVC_NAME}" 2>/dev/null || true
            done
            for rl in 0 1 6; do
                ln -sfn "../init.d/${SVC_NAME}" "/etc/rc${rl}.d/K01${SVC_NAME}" 2>/dev/null || true
            done
            [ -e "/etc/rc2.d/S99${SVC_NAME}" ] \
                || warn "sysvinit: failed to enable at boot; add ${SVC_NAME} to rc.local"
        fi
        if [ "$DO_START" = 1 ]; then
            "/etc/init.d/${SVC_NAME}" restart \
                || warn "failed to start $SVC_NAME: see $LOG_DIR/${SVC_NAME}.out"
        fi
        ;;
    slackware)
        render_service "$SCRIPT_DIR/init/sysvinit/service.template" "$tmp/initd"
        install -D -m 0755 "$tmp/initd" "/etc/rc.d/rc.${SVC_NAME}"
        if [ -f /etc/rc.d/rc.local ] && ! grep -q "rc\.${SVC_NAME} start" /etc/rc.d/rc.local; then
            printf '\n[ -x /etc/rc.d/rc.%s ] && /etc/rc.d/rc.%s start\n' \
                "$SVC_NAME" "$SVC_NAME" >> /etc/rc.d/rc.local
        fi
        if [ "$DO_START" = 1 ]; then
            "/etc/rc.d/rc.${SVC_NAME}" restart \
                || warn "failed to start $SVC_NAME: see $LOG_DIR/${SVC_NAME}.out"
        fi
        ;;
    *)
        install -d -m 0755 "$STATE_DIR/init"
        render_service "$SCRIPT_DIR/init/sysvinit/service.template" "$STATE_DIR/init/${SVC_NAME}"
        warn "unknown init system; script saved to $STATE_DIR/init/${SVC_NAME}"
        warn "start it manually or add it to your supervisor"
        ;;
    esac
    rm -rf "$tmp"
}

install_client_wrapper() {
    local client_bin="$1" tmp
    tmp="$(mktemp)"
    cat > "$tmp" <<EOF
#!/bin/sh
APP_DIR="$APP_DIR"
CLIENT="$client_bin"
X11_MODE="$FORCE_X11"

has_wayland_plugin() {
    for dir in "\$APP_DIR/plugins/platforms" "\$APP_DIR/lib/plugins/platforms" "\$APP_DIR/plugins"; do
        for plugin in "\$dir"/libqwayland*; do
            [ -e "\$plugin" ] && return 0
        done
    done
    return 1
}

if [ -z "\${QT_QPA_PLATFORM:-}" ]; then
    if [ "\$X11_MODE" = "yes" ]; then
        export QT_QPA_PLATFORM=xcb
    elif [ "\$X11_MODE" = "auto" ] && [ "\${XDG_SESSION_TYPE:-}" = "wayland" ]; then
        has_wayland_plugin || export QT_QPA_PLATFORM=xcb
    fi
fi

exec "\$CLIENT" "\$@"
EOF
    install -D -m 0755 "$tmp" /usr/local/bin/AmneziaVPN
    rm -f "$tmp"
}

install_desktop_entry() {
    local icon=""
    for candidate in \
        "$APP_DIR/AmneziaVPN.png" \
        "$APP_DIR/AmneziaVPN.svg" \
        "$APP_DIR/icon.png" \
        "$APP_DIR/bin/AmneziaVPN.png" \
        "$SCRIPT_DIR/assets/AmneziaVPN.png"; do
        [ -f "$candidate" ] && { icon="$candidate"; break; }
    done

    if [ -n "$icon" ]; then
        case "$icon" in
            *.svg)
                install -D -m 0644 "$icon" /usr/share/icons/hicolor/scalable/apps/AmneziaVPN.svg
                ;;
            *)
                install -D -m 0644 "$icon" /usr/share/icons/hicolor/512x512/apps/AmneziaVPN.png
                install -D -m 0644 "$icon" /usr/share/pixmaps/AmneziaVPN.png
                ;;
        esac
    fi

    install -d -m 0755 /usr/share/applications
    cat > /usr/share/applications/AmneziaVPN.desktop <<'EOF'
[Desktop Entry]
Type=Application
Name=AmneziaVPN
Comment=Amnezia VPN client
Exec=/usr/local/bin/AmneziaVPN
Icon=AmneziaVPN
Terminal=false
Categories=Network;VPN;Security;
StartupWMClass=AmneziaVPN
EOF
    chmod 0644 /usr/share/applications/AmneziaVPN.desktop
}

if [ "$CHECK_ONLY" = 0 ]; then
    need_root "$@"
fi

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

INIT="$(detect_init)"
if [ "$INIT" = "systemd" ]; then
    if [ "$CHECK_ONLY" = 1 ]; then
        log "init system: systemd (handled by post_install.sh)"
        exit 0
    fi
    die "systemd is handled by post_install.sh; use the package installer"
fi
log "init system: $INIT"

[ -d "$APP_DIR" ] || die "directory not found: $APP_DIR. Pass --app-dir."
SERVICE_BIN="$(find_service_binary "$APP_DIR")" \
    || die "AmneziaVPN-service not found in $APP_DIR"
CLIENT_BIN="$(find_client_binary "$APP_DIR" || true)"
log "service: $SERVICE_BIN"
[ -n "$CLIENT_BIN" ] && log "client:  $CLIENT_BIN"

if [ "$CHECK_ONLY" = 1 ]; then
    log "DNS: handled by the daemon (resolvconf or /etc/resolv.conf)"
    if [ -n "$CLIENT_BIN" ]; then
        if [ "$FORCE_X11" = "yes" ]; then
            log "GUI: XWayland forced"
        elif has_wayland_plugin "$APP_DIR"; then
            log "GUI: Wayland plugin found"
        else
            log "GUI: no Wayland plugin, XWayland will be used on Wayland sessions"
        fi
    fi
    log "check mode: no changes made"
    exit 0
fi

install -d -m 0755 "$LOG_DIR" "$STATE_DIR"

if [ -f /etc/systemd/system/AmneziaVPN.service ]; then
    rm -f /etc/systemd/system/AmneziaVPN.service
    log "removed stale /etc/systemd/system/AmneziaVPN.service"
fi

pkill -f 'bin/AmneziaVPN-service' 2>/dev/null || true
pkill -f 'AmneziaVPN-service$' 2>/dev/null || true
sleep 1

SVC_NAME="$SERVICE_NAME"
SVC_DESC="AmneziaVPN privileged service"
SVC_BIN="$SERVICE_BIN"
SVC_ARGS=""
SVC_OPENRC_DEPS="use net; use dbus"
SVC_DINIT_DEPS=""
install_service

if [ -n "$CLIENT_BIN" ]; then
    install_client_wrapper "$CLIENT_BIN"
    install_desktop_entry
fi

log "done"
cat <<EOF

Check:
  init=$INIT
  sudo rc-service ${SERVICE_NAME} status   # OpenRC
  sudo sv status ${SERVICE_NAME}           # runit
  sudo dinitctl status ${SERVICE_NAME}     # dinit
  tail -f ${LOG_DIR}/amneziavpn.log             # OpenRC
  tail -f ${LOG_DIR}/runit-amneziavpn/current   # runit

Launch: AmneziaVPN (or /usr/local/bin/AmneziaVPN)
EOF
