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

# Cloak config
sudo docker exec -i $CONTAINER_NAME bash -c '\
mkdir -p /opt/amnezia/cloak; \
cd /opt/amnezia/cloak || exit 1; \
CLOAK_ADMIN_UID=$(ck-server -u) && echo $CLOAK_ADMIN_UID > /opt/amnezia/cloak/cloak_admin_uid.key; \
CLOAK_BYPASS_UID=$(ck-server -u) && echo $CLOAK_BYPASS_UID > /opt/amnezia/cloak/cloak_bypass_uid.key; \
IFS=, read CLOAK_PUBLIC_KEY CLOAK_PRIVATE_KEY <<<$(ck-server -k); \
echo $CLOAK_PUBLIC_KEY > /opt/amnezia/cloak/cloak_public.key; \
echo $CLOAK_PRIVATE_KEY > /opt/amnezia/cloak/cloak_private.key; \
echo -e "{\\n\
  \"ProxyBook\": {\\n\
     \"openvpn\": [\\n\
      \"$OPENVPN_TRANSPORT_PROTO\",\\n\
      \"localhost:$OPENVPN_PORT\"\\n\
    ]\\n\
  },\\n\
  \"BypassUID\": [\\n\
    \"$CLOAK_BYPASS_UID\"\\n\
  ],\\n\
  \"BindAddr\":[\":443\"],\\n\
  \"RedirAddr\": \"$FAKE_WEB_SITE_ADDRESS\",\\n\
  \"PrivateKey\": \"$CLOAK_PRIVATE_KEY\",\\n\
  \"AdminUID\": \"$CLOAK_ADMIN_UID\",\\n\
  \"DatabasePath\": \"userinfo.db\",\\n\
  \"StreamTimeout\": 300\\n\
}" >/opt/amnezia/cloak/ck-config.json'
