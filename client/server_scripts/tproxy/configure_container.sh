#!/bin/sh

echo "[*] Amnezia TProxy: configure script start"
echo "[*] TPROXY_HOSTNAME=${TPROXY_HOSTNAME:-}"
echo "[*] TPROXY_ACME_EMAIL=${TPROXY_ACME_EMAIL:-}"
echo "[*] TPROXY_HTTPS_PORT=${TPROXY_HTTPS_PORT:-}"
echo "[*] TPROXY_HTTP_PORT=${TPROXY_HTTP_PORT:-}"
echo "[*] TPROXY_CARRIER_MODE=${TPROXY_CARRIER_MODE:-}"
echo "[*] TPROXY_WORKERS=${TPROXY_WORKERS:-}"
echo "[*] TPROXY_REGENERATE_SECRET=${TPROXY_REGENERATE_SECRET:-0}"
mkdir -p /data/site /data/caddy

if [ "$TPROXY_REGENERATE_SECRET" = "1" ]; then
    SECRET=$(openssl rand -hex 16)
    echo "[*] Secret source: regenerate (fresh install)"
elif [ -n "$TPROXY_SECRET" ]; then
    SECRET="$TPROXY_SECRET"
    echo "[*] Secret source: TPROXY_SECRET env"
elif [ -f /data/secret ]; then
    SECRET=$(cat /data/secret)
    echo "[*] Secret source: /data/secret file"
else
    SECRET=$(openssl rand -hex 16)
    echo "[*] Secret source: new random"
fi
echo "$SECRET" | grep -qE '^[0-9a-fA-F]{32}$' || SECRET=$(openssl rand -hex 16)

HOST=$TPROXY_HOSTNAME
EMAIL=$TPROXY_ACME_EMAIL
CARRIER=$TPROXY_CARRIER_MODE
[ -z "$CARRIER" ] && CARRIER=https
WORKERS=$TPROXY_WORKERS
[ -z "$WORKERS" ] && WORKERS=1
HTTPS_PORT=$TPROXY_HTTPS_PORT
[ -z "$HTTPS_PORT" ] && HTTPS_PORT=443
HTTP_PORT=$TPROXY_HTTP_PORT
[ -z "$HTTP_PORT" ] && HTTP_PORT=80

if [ -z "$HOST" ] || [ -z "$EMAIL" ]; then
    echo "[!] ERROR: TPROXY_HOSTNAME and TPROXY_ACME_EMAIL are required"
    exit 1
fi

echo "$SECRET" > /data/secret
chmod 600 /data/secret 2>/dev/null || true

{
    printf 'hostname=%s\n' "$HOST"
    printf 'email=%s\n'    "$EMAIL"
    printf 'carrier=%s\n'  "$CARRIER"
    printf 'workers=%s\n'  "$WORKERS"
    printf 'http_port=%s\n' "$HTTP_PORT"
    printf 'https_port=%s\n' "$HTTPS_PORT"
} > /data/tproxy-meta

# Decoy landing: unique enough per hostname/secret, not a shared Amnezia fingerprint page.
SITE_ID=$(echo -n "${HOST}${SECRET}" | openssl dgst -sha256 | awk '{print substr($2,1,12)}')
cat > /data/site/index.html << SITE
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>${HOST}</title>
<style>
body { font-family: Georgia, serif; margin: 12% auto; max-width: 36em; color: #222; }
h1 { font-weight: normal; font-size: 1.4em; }
p { line-height: 1.5; color: #555; }
</style>
</head>
<body>
<h1>${HOST}</h1>
<p>This host is reserved. Ref ${SITE_ID}.</p>
</body>
</html>
SITE

cat > /data/config.json << EOF
{
  "public_hostname": "$HOST",
  "listen": "127.0.0.1:8080",
  "admin_listen": "127.0.0.1:8081",
  "public_dir": "/data/site",
  "profiles_file": "/data/profiles.json",
  "enable_pprof": false
}
EOF

cat > /data/profiles.json << EOF
{
  "profiles": [
    {
      "name": "default",
      "secret": "$SECRET",
      "backend": "127.0.0.1:2398",
      "carrier_mode": "$CARRIER"
    }
  ]
}
EOF
chmod 0400 /data/profiles.json

curl -s --max-time 15 https://core.telegram.org/getProxySecret -o /data/proxy-secret
if [ -s /data/proxy-secret ]; then
    echo "[*] proxy-secret downloaded ($(wc -c < /data/proxy-secret | tr -d ' ') bytes)"
else
    echo "[!] WARNING: proxy-secret download failed or empty"
fi
curl -s --max-time 15 https://core.telegram.org/getProxyConfig -o /data/proxy-multi.conf
if [ -s /data/proxy-multi.conf ]; then
    echo "[*] proxy-multi.conf downloaded ($(wc -c < /data/proxy-multi.conf | tr -d ' ') bytes)"
else
    echo "[!] WARNING: proxy-multi.conf download failed or empty"
fi

echo "[*] HTTP port:  ${HTTP_PORT} (host) -> 80 (container)"
echo "[*] HTTPS port: ${HTTPS_PORT} (host) -> 443 (container)"
echo "[*] Carrier:    ${CARRIER}"
echo "[*] Workers:    ${WORKERS}"
echo "[*] Site id:    ${SITE_ID}"

if [ -n "$EMAIL" ] && [ -n "$HOST" ]; then
    echo "[*] Caddy: ACME TLS for ${HOST}"
    cat > /data/Caddyfile << EOF
{
	email $EMAIL
	admin off
	servers {
		protocols h1 h2
		timeouts {
			read_header 10s
			read_body 60s
		}
	}
}

$HOST {
	reverse_proxy 127.0.0.1:8080 {
		transport http {
			read_timeout 40s
			write_timeout 40s
		}
		header_up X-Forwarded-For {remote_host}
		header_up X-Forwarded-Proto {scheme}
		header_up X-Forwarded-Host {host}
	}
}
EOF
else
    echo "[*] Caddy: HTTP-only fallback on container :80 (no ACME block)"
    cat > /data/Caddyfile << EOF
{
	auto_https off
	admin off
}
:80 {
	reverse_proxy 127.0.0.1:8080
}
EOF
fi

echo "[*] TProxy configuration"
echo "[*] Secret:    $SECRET"
echo "[*] Hostname:  $HOST"
if [ -n "$HOST" ]; then
    echo "[*] tg:// link:   tg://webproxy?server=${HOST}&secret=${SECRET}"
    echo "[*] t.me link:    https://t.me/webproxy?server=${HOST}&secret=${SECRET}"
else
    echo "[*] tg:// link:   "
    echo "[*] t.me link:    "
fi
