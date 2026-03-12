#!/bin/sh

# Download Telegram config files
curl -s https://core.telegram.org/getProxySecret -o /data/proxy-secret
curl -s https://core.telegram.org/getProxyConfig -o /data/proxy-multi.conf

# Determine secret: env var -> saved file -> generate new
if [ -n "$MTPROXY_SECRET" ]; then
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

# Determine tag
TAG=""
if [ -n "$MTPROXY_TAG" ]; then
    TAG="$MTPROXY_TAG"
fi

# Detect external IP
IP=$(curl -s --max-time 5 https://api.ipify.org 2>/dev/null)
[ -z "$IP" ] && IP=$(curl -s --max-time 5 https://ifconfig.me 2>/dev/null)
[ -z "$IP" ] && IP=$(curl -s --max-time 5 https://icanhazip.com 2>/dev/null)

PORT=$MTPROXY_PORT
FAKETLS_SECRET="dd${SECRET}"

# Output stable markers — parsed by updateContainerConfigAfterInstallation()
echo "[*] MTProxy configuration"
echo "[*] Secret:    ${SECRET}"
echo "[*] FakeTLS:   ${FAKETLS_SECRET}"
echo "[*] tg:// link:   tg://proxy?server=${IP}&port=${PORT}&secret=${FAKETLS_SECRET}"
echo "[*] t.me link:    https://t.me/proxy?server=${IP}&port=${PORT}&secret=${FAKETLS_SECRET}"
