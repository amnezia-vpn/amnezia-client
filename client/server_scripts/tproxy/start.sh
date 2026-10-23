#!/bin/sh

if [ ! -f /data/secret ]; then
    echo "ERROR: /data/secret not found — run configure_container first"
    tail -f /dev/null
    exit 1
fi

SECRET=$(cat /data/secret)
WORKERS=1
if [ -f /data/tproxy-meta ]; then
    _w=$(grep '^workers=' /data/tproxy-meta 2>/dev/null | head -1 | cut -d= -f2-)
    [ -n "$_w" ] && WORKERS="$_w"
fi

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

tproxy-server -config /data/config.json &
RPID=$!

caddy run --config /data/Caddyfile --adapter caddyfile &
CPID=$!

wait $MPID $RPID $CPID
