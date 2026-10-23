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

# Build domain argument for FakeTLS mode
DOMAIN_ARG=""
if [ "$MTPROXY_TRANSPORT_MODE" = "faketls" ] && [ -n "$MTPROXY_TLS_DOMAIN" ]; then
    DOMAIN_ARG="--domain $MTPROXY_TLS_DOMAIN"
fi

WORKERS=$MTPROXY_WORKERS
STATS_PORT=2398
LISTEN_PORT=$MTPROXY_PORT

NAT_FLAG=""
NAT_VALUE=""
if [ "$MTPROXY_NAT_ENABLED" = "1" ] && [ -n "$MTPROXY_NAT_INTERNAL_IP" ] && [ -n "$MTPROXY_NAT_EXTERNAL_IP" ]; then
    NAT_FLAG="--nat-info"
    NAT_VALUE="$MTPROXY_NAT_INTERNAL_IP:$MTPROXY_NAT_EXTERNAL_IP"
else
    INTERNAL_IP=$(hostname -i 2>/dev/null | awk '{print $1}')
    EXTERNAL_IP=$(curl -s --max-time 5 https://api.ipify.org 2>/dev/null)
    [ -z "$EXTERNAL_IP" ] && EXTERNAL_IP=$(curl -s --max-time 5 https://ifconfig.me 2>/dev/null)

    if [ -n "$INTERNAL_IP" ] && [ -n "$EXTERNAL_IP" ] && [ "$INTERNAL_IP" != "$EXTERNAL_IP" ]; then
        NAT_FLAG="--nat-info"
        NAT_VALUE="${INTERNAL_IP}:${EXTERNAL_IP}"
    fi
fi

# Build additional secrets arguments
ADDITIONAL_SECRETS_ARG=""
if [ -n "$MTPROXY_ADDITIONAL_SECRETS" ]; then
    for S in $(echo "$MTPROXY_ADDITIONAL_SECRETS" | tr ',' ' '); do
        ADDITIONAL_SECRETS_ARG="$ADDITIONAL_SECRETS_ARG -S $S"
    done
fi

# Start proxy (foreground)
exec mtproto-proxy \
    -u root \
    -p ${STATS_PORT} \
    -H ${LISTEN_PORT} \
    -S ${SECRET} \
    ${ADDITIONAL_SECRETS_ARG} \
    --aes-pwd /data/proxy-secret \
    -M ${WORKERS} \
    -C 60000 \
    --allow-skip-dh \
    ${NAT_FLAG:+${NAT_FLAG} ${NAT_VALUE}} \
    ${TAG_ARG} \
    ${DOMAIN_ARG} \
    /data/proxy-multi.conf
