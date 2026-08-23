#!/bin/sh

echo "Container startup (TProxy WEB proxy)"

if [ ! -f /data/secret ]; then
    echo "ERROR: /data/secret not found — run configure_container first"
    tail -f /dev/null
    exit 1
fi

SECRET=$(cat /data/secret)
WORKERS=1
HOSTNAME=""
HTTP_PORT=""
HTTPS_PORT=""
CARRIER=""
if [ -f /data/tproxy-meta ]; then
    _w=$(grep '^workers=' /data/tproxy-meta 2>/dev/null | head -1 | cut -d= -f2-)
    [ -n "$_w" ] && WORKERS="$_w"
    HOSTNAME=$(grep '^hostname=' /data/tproxy-meta 2>/dev/null | head -1 | cut -d= -f2-)
    HTTP_PORT=$(grep '^http_port=' /data/tproxy-meta 2>/dev/null | head -1 | cut -d= -f2-)
    HTTPS_PORT=$(grep '^https_port=' /data/tproxy-meta 2>/dev/null | head -1 | cut -d= -f2-)
    CARRIER=$(grep '^carrier=' /data/tproxy-meta 2>/dev/null | head -1 | cut -d= -f2-)
fi

echo "[*] TProxy start: hostname=${HOSTNAME:-?} http_port=${HTTP_PORT:-?} https_port=${HTTPS_PORT:-?}"
echo "[*] TProxy start: carrier=${CARRIER:-?} workers=${WORKERS} secret_len=${#SECRET}"

if [ ! -s /data/proxy-secret ]; then
    echo "[!] WARNING: /data/proxy-secret missing or empty"
fi
if [ ! -s /data/proxy-multi.conf ]; then
    echo "[!] WARNING: /data/proxy-multi.conf missing or empty"
fi
if [ ! -f /data/config.json ]; then
    echo "[!] WARNING: /data/config.json missing"
fi
if [ ! -f /data/Caddyfile ]; then
    echo "[!] WARNING: /data/Caddyfile missing"
fi

mkdir -p /data/caddy /data/site
export XDG_DATA_HOME=/data/caddy
export XDG_CONFIG_HOME=/data/caddy

stop_children() {
    echo "[*] TProxy stop: killing children mtproxy=$MPID relay=$RPID caddy=$CPID"
    kill $MPID $RPID $CPID 2>/dev/null
    wait
}
trap 'stop_children; exit 0' TERM INT

# Official MTProxy runs inside docker (container IP != public IP). Without --nat-info
# it fails middle-proxy registration and never opens upstream connections to Telegram
# DC, so the relay gets no downstream bytes. Mirror the standalone mtproxy launch.
NAT_FLAG=""
NAT_VALUE=""
INTERNAL_IP=$(hostname -i 2>/dev/null | awk '{print $1}')
# Amnezia injects the resolved public server IP; api.ipify from inside the container
# is unreliable (often empty) and an empty external IP breaks middle-proxy registration.
EXTERNAL_IP="$SERVER_IP_ADDRESS"
if [ -z "$EXTERNAL_IP" ]; then
    EXTERNAL_IP=$(curl -s --max-time 5 https://api.ipify.org 2>/dev/null)
    [ -z "$EXTERNAL_IP" ] && EXTERNAL_IP=$(curl -s --max-time 5 https://ifconfig.me 2>/dev/null)
fi
if [ -n "$INTERNAL_IP" ] && [ -n "$EXTERNAL_IP" ] && [ "$INTERNAL_IP" != "$EXTERNAL_IP" ]; then
    NAT_FLAG="--nat-info"
    # mtproto-proxy wants a single arg: --nat-info <local-addr>:<global-addr> (colon-separated).
    NAT_VALUE="${INTERNAL_IP}:${EXTERNAL_IP}"
fi
echo "[*] TProxy start: mtproto nat_info=${NAT_VALUE:-none}"

mtproto-proxy \
    -u root \
    -p 8888 \
    -H 2398 \
    -S ${SECRET} \
    --aes-pwd /data/proxy-secret \
    -M ${WORKERS} \
    -C 60000 \
    --allow-skip-dh \
    ${NAT_FLAG:+${NAT_FLAG} ${NAT_VALUE}} \
    /data/proxy-multi.conf &
MPID=$!
echo "[*] TProxy start: mtproto-proxy PID=$MPID (backend 127.0.0.1:2398)"

tproxy-server -config /data/config.json &
RPID=$!
echo "[*] TProxy start: tproxy-server PID=$RPID"

caddy run --config /data/Caddyfile --adapter caddyfile &
CPID=$!
echo "[*] TProxy start: caddy PID=$CPID"

wait $MPID $RPID $CPID
echo "[*] TProxy start: a child exited, shutting down"
