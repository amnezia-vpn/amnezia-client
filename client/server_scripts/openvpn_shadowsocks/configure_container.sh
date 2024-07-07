cat > /opt/amnezia/openvpn/server.conf <<EOF
port $OPENVPN_PORT
proto tcp
dev tun
ca /opt/amnezia/openvpn/ca.crt
cert /opt/amnezia/openvpn/AmneziaReq.crt
key /opt/amnezia/openvpn/AmneziaReq.key
dh /opt/amnezia/openvpn/dh.pem
server $OPENVPN_SUBNET_IP $OPENVPN_SUBNET_MASK
ifconfig-pool-persist ipp.txt
duplicate-cn
keepalive 10 120
$OPENVPN_NCP_DISABLE
cipher $OPENVPN_CIPHER
data-ciphers $OPENVPN_CIPHER
auth $OPENVPN_HASH
user nobody
group nobody
persist-key
persist-tun
crl-verify /opt/amnezia/openvpn/crl.pem
status openvpn-status.log
verb 1
tls-server
tls-version-min 1.2
$OPENVPN_TLS_AUTH
$OPENVPN_ADDITIONAL_SERVER_CONFIG
EOF

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
