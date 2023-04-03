# ShadowSocks config
mkdir -p /opt/amnezia/shadowsocks
cd /opt/amnezia/shadowsocks
SHADOWSOCKS_PASSWORD=$(openssl rand -base64 32 | tr "=" "A" | tr "+" "A" | tr "/" "A")
echo $SHADOWSOCKS_PASSWORD > /opt/amnezia/shadowsocks/shadowsocks.key

cat > /opt/amnezia/shadowsocks/ss-config.json <<EOF
{
    "local_port": 8585,
    "method": "$SHADOWSOCKS_CIPHER",
    "password": "$SHADOWSOCKS_PASSWORD",
    "server": "0.0.0.0",
    "server_port": $SHADOWSOCKS_SERVER_PORT,
    "timeout": 60,
    "mode" : "tcp_and_udp"
}
EOF
