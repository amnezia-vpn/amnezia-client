#!/bin/sh

echo "Container startup"

# Read persisted secret
SECRET=""
if [ -f /data/secret ]; then
    SECRET=$(cat /data/secret)
fi

if [ -z "$SECRET" ]; then
    echo "ERROR: /data/secret not found — run configure_container first"
    tail -f /dev/null
    exit 1
fi

# Build tag argument
TAG_ARG=""
if [ -n "$MTPROXY_TAG" ]; then
    TAG_ARG="-P $MTPROXY_TAG"
fi

WORKERS=2
STATS_PORT=2398

# Detect internal and external IPs for NAT
INTERNAL_IP=$(hostname -i 2>/dev/null | awk '{print $1}')
EXTERNAL_IP=$(curl -s --max-time 5 https://api.ipify.org 2>/dev/null)
[ -z "$EXTERNAL_IP" ] && EXTERNAL_IP=$(curl -s --max-time 5 https://ifconfig.me 2>/dev/null)

NAT_ARG=""
if [ -n "$INTERNAL_IP" ] && [ -n "$EXTERNAL_IP" ] && [ "$INTERNAL_IP" != "$EXTERNAL_IP" ]; then
    NAT_ARG="--nat-info ${INTERNAL_IP}:${EXTERNAL_IP}"
fi

# Start proxy (foreground)
exec mtproto-proxy \
    -u root \
    -p ${STATS_PORT} \
    -H 443 \
    -S ${SECRET} \
    --aes-pwd /data/proxy-secret \
    -M ${WORKERS} \
    -C 60000 \
    --allow-skip-dh \
    ${NAT_ARG} \
    ${TAG_ARG} \
    /data/proxy-multi.conf
