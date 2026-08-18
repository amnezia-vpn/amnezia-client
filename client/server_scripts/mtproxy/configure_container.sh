#!/bin/sh

# Download Telegram config files
curl -s https://core.telegram.org/getProxySecret -o /data/proxy-secret
curl -s https://core.telegram.org/getProxyConfig -o /data/proxy-multi.conf

# Determine secret: regenerate (fresh install) -> env var -> saved file -> generate new
if [ "$MTPROXY_REGENERATE_SECRET" = "1" ]; then
    SECRET=$(openssl rand -hex 16)
elif [ -n "$MTPROXY_SECRET" ]; then
    SECRET="$MTPROXY_SECRET"
elif [ -f /data/secret ]; then
    SECRET=$(cat /data/secret)
else
    SECRET=$(openssl rand -hex 16)
fi

# Validate: must be exactly 32 hex chars
echo "$SECRET" | grep -qE '^[0-9a-fA-F]{32}$' || SECRET=$(openssl rand -hex 16)

# Persist secret for start.sh restarts
echo "$SECRET" > /data/secret

# Detect external IP
IP=$(curl -s --max-time 5 https://api.ipify.org 2>/dev/null)
[ -z "$IP" ] && IP=$(curl -s --max-time 5 https://ifconfig.me 2>/dev/null)
[ -z "$IP" ] && IP=$(curl -s --max-time 5 https://icanhazip.com 2>/dev/null)

# Use custom public host/domain if provided, otherwise fall back to detected IP
if [ -n "$MTPROXY_PUBLIC_HOST" ]; then
    LINK_HOST="$MTPROXY_PUBLIC_HOST"
else
    LINK_HOST="$IP"
fi

PORT=$MTPROXY_PORT

# Transport mode is substituted by replaceVars — plain variable, no curly braces
TRANSPORT_MODE=$MTPROXY_TRANSPORT_MODE

PADDED_SECRET="dd${SECRET}"

if [ "$TRANSPORT_MODE" = "faketls" ] && [ -n "$MTPROXY_TLS_DOMAIN" ]; then
    DOMAIN_HEX=$(echo -n "$MTPROXY_TLS_DOMAIN" | od -A n -t x1 | tr -d ' \n')
    FAKETLS_SECRET="ee${SECRET}${DOMAIN_HEX}"
else
    FAKETLS_SECRET=""
fi

# Persist deployment state for restore on re-scan.
{
    printf 'mode=%s\n'          "$TRANSPORT_MODE"
    printf 'domain=%s\n'        "$MTPROXY_TLS_DOMAIN"
    printf 'tag=%s\n'           "$MTPROXY_TAG"
    printf 'additional=%s\n'    "$MTPROXY_ADDITIONAL_SECRETS"
    printf 'workers_mode=%s\n'  "$MTPROXY_WORKERS_MODE"
    printf 'workers=%s\n'       "$MTPROXY_WORKERS"
    printf 'nat_enabled=%s\n'   "$MTPROXY_NAT_ENABLED"
    printf 'nat_internal=%s\n'  "$MTPROXY_NAT_INTERNAL_IP"
    printf 'nat_external=%s\n'  "$MTPROXY_NAT_EXTERNAL_IP"
    printf 'public_host=%s\n'   "$MTPROXY_PUBLIC_HOST"
} > /data/mtproxy-meta

# Active link secret depends on transport mode
if [ "$TRANSPORT_MODE" = "faketls" ] && [ -n "$FAKETLS_SECRET" ]; then
    LINK_SECRET="$FAKETLS_SECRET"
else
    LINK_SECRET="$PADDED_SECRET"
fi

# Output stable markers — parsed by updateContainerConfigAfterInstallation()
echo "[*] MTProxy configuration"
echo "[*] Secret:    ${SECRET}"
echo "[*] FakeTLS:   ${FAKETLS_SECRET}"
echo "[*] tg:// link:   tg://proxy?server=${LINK_HOST}&port=${PORT}&secret=${LINK_SECRET}"
echo "[*] t.me link:    https://t.me/proxy?server=${LINK_HOST}&port=${PORT}&secret=${LINK_SECRET}"
