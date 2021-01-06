#DOCKER_IMAGE="amneziavpn/shadow-vpn:latest"
#CONTAINER_NAME="shadow-vpn"

#sudo apt update
sudo apt install -y docker.io curl
sudo systemctl start docker

sudo docker stop shadow-vpn
sudo docker rm -f shadow-vpn
sudo docker pull amneziavpn/shadow-vpn:latest
sudo docker run -d --restart always --cap-add=NET_ADMIN -p 1194:1194/tcp -p 6789:6789/tcp --name shadow-vpn amneziavpn/shadow-vpn:latest


