#!/bin/sh

mkdir -p /data

if [ ! -s /data/proxy.pem ]; then
    openssl req -x509 -newkey rsa:2048 -nodes -days 3650 \
        -subj "/CN=$(openssl rand -hex 8).net" \
        -keyout /data/proxy.key -out /data/proxy.crt 2>/dev/null
    cat /data/proxy.key /data/proxy.crt > /data/proxy.pem
    rm -f /data/proxy.key /data/proxy.crt
    chmod 600 /data/proxy.pem
fi

cat > /data/haproxy.cfg << EOF_CFG
global
    tune.bufsize 4096
    maxconn 27500
    spread-checks 5
    ssl-server-verify none

defaults
    mode tcp
    timeout client-fin 1s
    timeout server-fin 1s
    timeout connect 5s
    timeout client 200s
    timeout server 200s
    default-server inter 10s fastinter 1s downinter 3s error-limit 50

frontend chat
    bind ipv4@*:443 ssl crt /data/proxy.pem
    default_backend wa_chat

frontend media
    bind ipv4@*:587
    default_backend wa_media

backend wa_chat
    default-server check inter 60000 observe layer4 send-proxy
    server g_whatsapp_net_5222 g.whatsapp.net:5222

backend wa_media
    default-server check inter 60000 observe layer4
    server whatsapp_net_443 whatsapp.net:443
EOF_CFG

echo "[*] WhatsApp proxy configuration"
echo "[*] Chat port:  $WHATSAPP_PROXY_CHAT_PORT"
echo "[*] Media port: $WHATSAPP_PROXY_MEDIA_PORT"
