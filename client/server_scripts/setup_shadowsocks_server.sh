# CONTAINER_NAME=... this var will be set in ServerController
# Don't run commands in background like sh -c "openvpn &"

pm_apt="/usr/bin/apt-get"; pm_yum="/usr/bin/yum"; if [[ -f "$pm_apt" ]]; then pm=$pm_apt; else pm=$pm_yum; fi; if [[ ! -f "/usr/bin/sudo" ]]; then $pm update -y -q; $pm install -y -q sudo; fi
sudo iptables -P FORWARD ACCEPT

pm_apt="/usr/bin/apt-get"; pm_yum="/usr/bin/yum"; if [[ -f "$pm_apt" ]]; then pm=$pm_apt; else pm=$pm_yum; fi; sudo $pm update -y -q
pm_apt="/usr/bin/apt-get"; pm_yum="/usr/bin/yum"; if [[ -f "$pm_apt" ]]; then pm=$pm_apt; else pm=$pm_yum; fi; sudo $pm install -y -q curl
pm_apt="/usr/bin/apt-get"; pm_yum="/usr/bin/yum"; if [[ -f "$pm_apt" ]]; then sudo export DEBIAN_FRONTEND=noninteractive; sudo $pm_apt install -y -q docker.io; else sudo $pm_yum install -y -q docker; fi
sudo systemctl start docker

sudo docker stop $CONTAINER_NAME
sudo docker rm -f $CONTAINER_NAME
sudo docker pull amneziavpn/shadowsocks:latest
sudo docker run -d --restart always --cap-add=NET_ADMIN -p 6789:6789/tcp --name $CONTAINER_NAME amneziavpn/shadowsocks:latest

# Prevent to route packets outside of the container in case if server behind of the NAT
sudo docker exec -i $CONTAINER_NAME sh -c "ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up"

# OpenVpn
sudo docker exec -i $CONTAINER_NAME bash -c 'mkdir -p /opt/amneziavpn_data/clients;\
cd /opt/amneziavpn_data && easyrsa init-pki;\
cd /opt/amneziavpn_data && easyrsa gen-dh;\
cd /opt/amneziavpn_data && cp pki/dh.pem /etc/openvpn && easyrsa build-ca nopass << EOF yes EOF && easyrsa gen-req MyReq nopass << EOF2 yes EOF2;\
cd /opt/amneziavpn_data && easyrsa sign-req server MyReq << EOF3 yes EOF3;\
cd /opt/amneziavpn_data && openvpn --genkey --secret ta.key << EOF4;\
cd /opt/amneziavpn_data && cp pki/ca.crt pki/issued/MyReq.crt pki/private/MyReq.key ta.key /etc/openvpn'

sudo docker exec -d $CONTAINER_NAME sh -c "openvpn --config /etc/openvpn/server.conf"
