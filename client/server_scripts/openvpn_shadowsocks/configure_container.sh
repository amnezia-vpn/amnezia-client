sudo docker exec -i $CONTAINER_NAME bash -c '\
echo -e "\
port $OPENVPN_PORT \\n\
proto tcp \\n\
dev tun \\n\
ca /opt/amnezia/openvpn/ca.crt \\n\
cert /opt/amnezia/openvpn/AmneziaReq.crt \\n\
key /opt/amnezia/openvpn/AmneziaReq.key \\n\
dh /opt/amnezia/openvpn/dh.pem \\n\
server $VPN_SUBNET_IP $VPN_SUBNET_MASK \\n\
ifconfig-pool-persist ipp.txt \\n\
duplicate-cn \\n\
keepalive 10 120 \\n\
$OPENVPN_NCP_DISABLE \\n\
cipher $OPENVPN_CIPHER \\n\
data-ciphers $OPENVPN_CIPHER \\n\
auth $OPENVPN_HASH \\n\
user nobody \\n\
group nobody \\n\
persist-key \\n\
persist-tun \\n\
status openvpn-status.log \\n\
verb 1 \\n\
tls-server \\n\
tls-version-min 1.2 \\n\
$OPENVPN_TLS_AUTH" >/opt/amnezia/openvpn/server.conf'

# Cloak config
sudo docker exec -i $CONTAINER_NAME bash -c '\
mkdir -p /opt/amnezia/shadowsocks; \
cd /opt/amnezia/shadowsocks || exit 1; \
SHADOWSOCKS_PASSWORD=$(openssl rand -base64 32 | tr "=" "A" | tr "+" "A" | tr "/" "A") && echo $SHADOWSOCKS_PASSWORD > /opt/amnezia/shadowsocks/shadowsocks.key; \
echo -e "{\\n\
    \"local_port\": 8585,\\n\
    \"method\": \"$SHADOWSOCKS_CIPHER\",\\n\
    \"password\": \"$SHADOWSOCKS_PASSWORD\",\\n\
    \"server\": \"0.0.0.0\",\\n\
    \"server_port\": $SHADOWSOCKS_SERVER_PORT,\\n\
    \"timeout\": 60\\n\
}" >/opt/amnezia/shadowsocks/ss-config.json'
