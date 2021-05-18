sudo docker exec -i $CONTAINER_NAME bash -c '\
echo -e "\
port $OPENVPN_PORT \\n\
proto $OPENVPN_TRANSPORT_PROTO \\n\
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

