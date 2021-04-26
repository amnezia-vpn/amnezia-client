# CONTAINER_NAME=... this var will be set in ServerController
# Don't run commands in background like sh -c "openvpn &"
# SERVER_PORT=443

#sudo docker stop $CONTAINER_NAME
#sudo docker rm -f $CONTAINER_NAME
#sudo docker pull amneziavpn/openvpn-cloak:latest
#sudo docker run -d --restart always --cap-add=NET_ADMIN -p $DOCKER_PORT:443/tcp --name $CONTAINER_NAME amneziavpn/openvpn-cloak:latest

sudo docker stop $CONTAINER_NAME
sudo docker rm -f $CONTAINER_NAME
sudo docker run -d --restart always --cap-add=NET_ADMIN -p $DOCKER_PORT:443/tcp --name $CONTAINER_NAME $CONTAINER_NAME

# Create tun device if not exist
sudo docker exec -i $CONTAINER_NAME bash -c 'mkdir -p /dev/net; if [ ! -c /dev/net/tun ]; then mknod /dev/net/tun c 10 200; fi'

# Prevent to route packets outside of the container in case if server behind of the NAT
sudo docker exec -i $CONTAINER_NAME sh -c "ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up"

# OpenVPN config
sudo docker exec -i $CONTAINER_NAME bash -c 'mkdir -p /opt/amnezia/openvpn/clients; \
cd /opt/amnezia/openvpn && easyrsa init-pki; \
cd /opt/amnezia/openvpn && easyrsa gen-dh; \
cd /opt/amnezia/openvpn && cp pki/dh.pem /opt/amnezia/openvpn && easyrsa build-ca nopass << EOF yes EOF && easyrsa gen-req AmneziaReq nopass << EOF2 yes EOF2;\
cd /opt/amnezia/openvpn && easyrsa sign-req server AmneziaReq << EOF3 yes EOF3;\
cd /opt/amnezia/openvpn && openvpn --genkey --secret ta.key << EOF4;\
cd /opt/amnezia/openvpn && cp pki/ca.crt pki/issued/AmneziaReq.crt pki/private/AmneziaReq.key /opt/amnezia/openvpn'

sudo docker exec -i $CONTAINER_NAME bash -c '\
echo -e "\
port 1194 \\n\
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
cipher AES-256-GCM \\n\
ncp-ciphers AES-256-GCM:AES-256-CBC \\n\
auth SHA512 \\n\
user nobody \\n\
group nobody \\n\
persist-key \\n\
persist-tun \\n\
status openvpn-status.log \\n\
verb 1 \\n\
tls-server \\n\
tls-version-min 1.2 \\n\
tls-auth /opt/amnezia/openvpn/ta.key 0" >>/opt/amnezia/openvpn/server.conf'

#sudo docker exec -d $CONTAINER_NAME sh -c "openvpn --config /opt/amnezia/openvpn/server.conf"

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
      \"tcp\",\\n\
      \"localhost:1194\"\\n\
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
}" >>/opt/amnezia/cloak/ck-config.json'

#sudo docker exec -d $CONTAINER_NAME sh -c "/usr/bin/ck-server -c /opt/amnezia/cloak/ck-config.json"
