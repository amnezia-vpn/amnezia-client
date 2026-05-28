#!/bin/sh
# Do not use set -e: Telemt / curl / kill edge cases should not abort the whole configure step.

echo "[*] Amnezia Telemt: configure script start"
mkdir -p /data/tlsfront

# Secret: regenerate (fresh install) -> env var -> saved file -> openssl
if [ "$TELEMT_REGENERATE_SECRET" = "1" ]; then
    SECRET=$(openssl rand -hex 16)
elif [ -n "$TELEMT_SECRET" ]; then
    SECRET="$TELEMT_SECRET"
elif [ -f /data/secret ]; then
    SECRET=$(cat /data/secret)
else
    SECRET=$(openssl rand -hex 16)
fi
# Must be exactly 32 hex chars
echo "$SECRET" | grep -qE '^[0-9a-fA-F]{32}$' || SECRET=$(openssl rand -hex 16)

# Build config.toml (other variables substituted on the host by Amnezia before upload)
rm -f /data/config.toml

{
    echo "### Amnezia Telemt — generated"
    echo "[general]"
    echo "use_middle_proxy = $TELEMT_USE_MIDDLE_PROXY"
    echo "log_level = \"normal\""
    if [ -n "$TELEMT_TAG" ]; then
        echo "ad_tag = \"$TELEMT_TAG\""
    fi
    echo ""
    echo "[general.modes]"
    echo "classic = false"
    echo "secure = $TELEMT_TOML_SECURE"
    echo "tls = $TELEMT_TOML_TLS"
    echo ""
    echo "[general.links]"
    echo "show = \"*\""
    if [ -n "$TELEMT_PUBLIC_HOST" ]; then
        echo "public_host = \"$TELEMT_PUBLIC_HOST\""
    fi
    echo "public_port = $TELEMT_PORT"
    echo ""
    echo "[server]"
    echo "port = $TELEMT_PORT"
    echo ""
    echo "[server.api]"
    echo "enabled = true"
    echo "listen = \"0.0.0.0:9091\""
    # Match upstream Telemt default: localhost API only (curl in this script uses 127.0.0.1).
    echo "whitelist = [\"127.0.0.0/8\"]"
    echo ""
    echo "[[server.listeners]]"
    echo "ip = \"0.0.0.0\""
    echo ""
    echo "[censorship]"
    echo "tls_domain = \"$TELEMT_TLS_DOMAIN\""
    echo "mask = $TELEMT_MASK"
    echo "tls_emulation = $TELEMT_TLS_EMULATION"
    echo "tls_front_dir = \"/data/tlsfront\""
    echo ""
    echo "[access.users]"
    echo "$TELEMT_USER_NAME = \"$SECRET\""
} > /data/config.toml

echo "$SECRET" > /data/secret
chmod 600 /data/secret 2>/dev/null || true

# Do not start telemt here: a long-lived process + curl loop inside `docker exec` can confuse SSH/Docker
# timing and is unnecessary — start.sh runs telemt after configure. Links can be empty until the service
# is up; the client still parses Secret below.
echo "[*] Telemt configuration"
echo "[*] Secret:    $SECRET"
echo "[*] tg:// link:   "
echo "[*] t.me link:    "
