# Run container
sudo docker run -d --restart always --cap-add=NET_ADMIN -p $CLOAK_SERVER_PORT:443/tcp --name $CONTAINER_NAME $CONTAINER_NAME

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
