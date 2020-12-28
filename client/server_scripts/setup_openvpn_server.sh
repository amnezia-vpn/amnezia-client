DOCKER_IMAGE="amneziavpn/openvpn:latest"
CONTAINER_NAME="amneziavpn"

#sudo apt update
sudo apt install -y docker.io curl
sudo systemctl start docker

sudo docker stop amneziavpn
sudo docker rm -f amneziavpn
sudo docker pull amneziavpn/openvpn:latest
sudo docker run -d --restart always --cap-add=NET_ADMIN -p 1194:1194/udp --name amneziavpn amneziavpn/openvpn:latest


docker exec -i amneziavpn sh -c "mkdir -p /opt/amneziavpn_data/clients"
docker exec -i amneziavpn sh -c "cat /proc/sys/kernel/random/entropy_avail"
docker exec -i amneziavpn sh -c "cd /opt/amneziavpn_data && easyrsa init-pki && easyrsa gen-dh"

docker exec -i amneziavpn sh -c "cd /opt/amneziavpn_data && cp pki/dh.pem /etc/openvpn && easyrsa build-ca nopass << EOF yes EOF && easyrsa gen-req MyReq nopass << EOF2 yes EOF2"
docker exec -i amneziavpn sh -c "cd /opt/amneziavpn_data && easyrsa sign-req server MyReq << EOF3 yes EOF3"
docker exec -i amneziavpn sh -c "openvpn --genkey --secret ta.key << EOF4"
docker exec -i amneziavpn sh -c "cd /opt/amneziavpn_data && cp pki/ca.crt pki/issued/MyReq.crt pki/private/MyReq.key ta.key /etc/openvpn"
docker exec -i amneziavpn sh -c "openvpn --config /etc/openvpn/server.conf &"
