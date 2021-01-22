#CONTAINER_NAME=... this var will be set in ServerController

apt-get update

iptables -P FORWARD ACCEPT

apt install -y docker.io curl
systemctl start docker

docker stop $CONTAINER_NAME
docker rm -f $CONTAINER_NAME
docker pull amneziavpn/openvpn:latest
docker run -d --restart always --cap-add=NET_ADMIN -p 1194:1194/udp --name $CONTAINER_NAME amneziavpn/openvpn:latest


docker exec -i $CONTAINER_NAME sh -c "mkdir -p /opt/amneziavpn_data/clients"
docker exec -i $CONTAINER_NAME sh -c "cd /opt/amneziavpn_data && easyrsa init-pki"
docker exec -i $CONTAINER_NAME sh -c "cd /opt/amneziavpn_data && easyrsa gen-dh"

docker exec -i $CONTAINER_NAME sh -c "cd /opt/amneziavpn_data && cp pki/dh.pem /etc/openvpn && easyrsa build-ca nopass << EOF yes EOF && easyrsa gen-req MyReq nopass << EOF2 yes EOF2"
docker exec -i $CONTAINER_NAME sh -c "cd /opt/amneziavpn_data && easyrsa sign-req server MyReq << EOF3 yes EOF3"
docker exec -i $CONTAINER_NAME sh -c "cd /opt/amneziavpn_data && openvpn --genkey --secret ta.key << EOF4"
docker exec -i $CONTAINER_NAME sh -c "cd /opt/amneziavpn_data && cp pki/ca.crt pki/issued/MyReq.crt pki/private/MyReq.key ta.key /etc/openvpn"
docker exec -d $CONTAINER_NAME sh -c "openvpn --config /etc/openvpn/server.conf"
