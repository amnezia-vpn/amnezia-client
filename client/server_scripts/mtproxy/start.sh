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
TICK_SECONDS=10
IP_CHECK_SECONDS=300
CONFIG_REFRESH_SECONDS=86400

is_ipv4() {
    echo "$1" | grep -qE '^[0-9]{1,3}(\.[0-9]{1,3}){3}$'
}

detect_auto_nat_value() {
    INTERNAL_IP=$(hostname -i 2>/dev/null | awk '{print $1}')
    EXTERNAL_IP=$(curl -s --max-time 5 https://api.ipify.org 2>/dev/null)
    is_ipv4 "$EXTERNAL_IP" || EXTERNAL_IP=$(curl -s --max-time 5 https://ifconfig.me 2>/dev/null)

    if [ -n "$INTERNAL_IP" ] && is_ipv4 "$EXTERNAL_IP" && [ "$INTERNAL_IP" != "$EXTERNAL_IP" ]; then
        echo "${INTERNAL_IP}:${EXTERNAL_IP}"
    fi
}

AUTO_NAT=0
NAT_VALUE=""
if [ "$MTPROXY_NAT_ENABLED" = "1" ] && [ -n "$MTPROXY_NAT_INTERNAL_IP" ] && [ -n "$MTPROXY_NAT_EXTERNAL_IP" ]; then
    NAT_VALUE="$MTPROXY_NAT_INTERNAL_IP:$MTPROXY_NAT_EXTERNAL_IP"
else
    AUTO_NAT=1
    NAT_VALUE=$(detect_auto_nat_value)
fi

# Build additional secrets arguments
ADDITIONAL_SECRETS_ARG=""
if [ -n "$MTPROXY_ADDITIONAL_SECRETS" ]; then
    for S in $(echo "$MTPROXY_ADDITIONAL_SECRETS" | tr ',' ' '); do
        ADDITIONAL_SECRETS_ARG="$ADDITIONAL_SECRETS_ARG -S $S"
    done
fi

start_proxy() {
    mtproto-proxy \
        -u root \
        -p ${STATS_PORT} \
        -H ${LISTEN_PORT} \
        -S ${SECRET} \
        ${ADDITIONAL_SECRETS_ARG} \
        --aes-pwd /data/proxy-secret \
        -M ${WORKERS} \
        -C 60000 \
        --allow-skip-dh \
        ${NAT_VALUE:+--nat-info ${NAT_VALUE}} \
        ${TAG_ARG} \
        ${DOMAIN_ARG} \
        /data/proxy-multi.conf &
    PROXY_PID=$!
}

stop_proxy() {
    kill -TERM "$PROXY_PID" 2>/dev/null
    wait "$PROXY_PID" 2>/dev/null
}

proxy_alive() {
    [ -r "/proc/$PROXY_PID/stat" ] && [ "$(cut -d' ' -f3 "/proc/$PROXY_PID/stat")" != "Z" ]
}

refresh_proxy_config() {
    curl -s --max-time 15 https://core.telegram.org/getProxyConfig -o /data/proxy-multi.conf.new 2>/dev/null
    if grep -q '^proxy_for ' /data/proxy-multi.conf.new 2>/dev/null && ! cmp -s /data/proxy-multi.conf.new /data/proxy-multi.conf; then
        mv /data/proxy-multi.conf.new /data/proxy-multi.conf
        echo "proxy-multi.conf updated, reloading"
        kill -HUP "$PROXY_PID"
    else
        rm -f /data/proxy-multi.conf.new
    fi
}

trap 'stop_proxy; exit 0' TERM INT

start_proxy

SINCE_IP_CHECK=0
SINCE_CONFIG_REFRESH=0
while true; do
    sleep "$TICK_SECONDS" &
    wait $!

    if ! proxy_alive; then
        echo "mtproto-proxy exited, restarting"
        wait "$PROXY_PID" 2>/dev/null
        start_proxy
        continue
    fi

    SINCE_IP_CHECK=$((SINCE_IP_CHECK + TICK_SECONDS))
    if [ "$AUTO_NAT" = "1" ] && [ "$SINCE_IP_CHECK" -ge "$IP_CHECK_SECONDS" ]; then
        SINCE_IP_CHECK=0
        NEW_NAT_VALUE=$(detect_auto_nat_value)
        if [ -n "$NEW_NAT_VALUE" ] && [ "$NEW_NAT_VALUE" != "$NAT_VALUE" ]; then
            echo "External IP changed, restarting mtproto-proxy"
            NAT_VALUE="$NEW_NAT_VALUE"
            stop_proxy
            start_proxy
        fi
    fi

    SINCE_CONFIG_REFRESH=$((SINCE_CONFIG_REFRESH + TICK_SECONDS))
    if [ "$SINCE_CONFIG_REFRESH" -ge "$CONFIG_REFRESH_SECONDS" ]; then
        SINCE_CONFIG_REFRESH=0
        refresh_proxy_config
    fi
done
