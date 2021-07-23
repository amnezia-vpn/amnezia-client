# Wireguard config
sudo docker exec -i $CONTAINER_NAME bash -c '\
mkdir -p /opt/amnezia/wireguard; \
cd /opt/amnezia/wireguard || exit 1; \
WIREGUARD_SERVER_PRIVATE_KEY=$(wg genkey) && echo $WIREGUARD_SERVER_PRIVATE_KEY > /opt/amnezia/wireguard/wireguard_server_private_key.key; \
WIREGUARD_SERVER_PUBLIC_KEY=$(echo $WIREGUARD_SERVER_PRIVATE_KEY | wg pubkey) && echo $WIREGUARD_SERVER_PUBLIC_KEY > /opt/amnezia/wireguard/wireguard_server_public_key.key; \
WIREGUARD_PSK=$(wg genpsk) && echo $WIREGUARD_PSK > /opt/amnezia/wireguard/wireguard_psk.key; \
echo -e "\
[Interface]\\n\
PrivateKey = $WIREGUARD_SERVER_PRIVATE_KEY \\n\
Address = $WIREGUARD_SUBNET_IP/$WIREGUARD_SUBNET_CIDR \\n\
ListenPort = $WIREGUARD_SERVER_PORT \\n\
" >/opt/amnezia/wireguard/wg0.conf'
