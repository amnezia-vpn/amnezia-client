#!/bin/bash
set -e

# Configure HTTPS reverse proxy with self-signed cert and set CryptPad origin
# Variables provided by the client before execution: $SERVER_IP_ADDRESS, $CRYPTPAD_PORT

CONFIG_DIR="/cryptpad/config"
CONFIG_FILE="$CONFIG_DIR/config.js"
NGINX_SSL_DIR="/etc/nginx/ssl"
NGINX_CONF_DIR="/etc/nginx/conf.d"
NGINX_SITE_CONF="$NGINX_CONF_DIR/cryptpad.conf"
CRT_FILE="$NGINX_SSL_DIR/cryptpad.crt"
KEY_FILE="$NGINX_SSL_DIR/cryptpad.key"

mkdir -p "$CONFIG_DIR"

# 1) Generate self-signed certificate if missing
mkdir -p "$NGINX_SSL_DIR" "$NGINX_CONF_DIR"
if [ ! -f "$CRT_FILE" ] || [ ! -f "$KEY_FILE" ]; then
  openssl req -x509 -newkey rsa:2048 -nodes \
    -keyout "$KEY_FILE" -out "$CRT_FILE" -days 3650 \
    -subj "/CN=$SERVER_IP_ADDRESS"
fi

# 2) Configure nginx as HTTPS reverse proxy to CryptPad (port 3000)
cat > "$NGINX_SITE_CONF" <<EOF
server {
  listen 443 ssl;
  server_name $SERVER_IP_ADDRESS;

  ssl_certificate     $CRT_FILE;
  ssl_certificate_key $KEY_FILE;

  # Recommended minimal settings
  ssl_protocols TLSv1.2 TLSv1.3;
  ssl_ciphers HIGH:!aNULL:!MD5;

  location / {
    proxy_pass http://127.0.0.1:3000;
    proxy_http_version 1.1;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_set_header X-Forwarded-Proto https;
  }
}
EOF

# 3) Configure CryptPad to trust HTTPS origin behind proxy
cat > "$CONFIG_FILE" <<EOF
module.exports = {
  // Public origin for HTTPS access. Must include trailing slash.
  httpSafeOrigin: 'https://$SERVER_IP_ADDRESS:$CRYPTPAD_PORT/',
  behindProxy: true
};
EOF

echo "CryptPad config written to $CONFIG_FILE"

# 4) Start or reload nginx to apply config
if pgrep -x nginx >/dev/null 2>&1; then
  nginx -s reload || true
else
  nginx || true
fi
