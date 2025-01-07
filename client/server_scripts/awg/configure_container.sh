mkdir -p /opt/amnezia/awg
cd /opt/amnezia/awg
WIREGUARD_SERVER_PRIVATE_KEY=$(wg genkey)
echo $WIREGUARD_SERVER_PRIVATE_KEY > /opt/amnezia/awg/wireguard_server_private_key.key

WIREGUARD_SERVER_PUBLIC_KEY=$(echo $WIREGUARD_SERVER_PRIVATE_KEY | wg pubkey)
echo $WIREGUARD_SERVER_PUBLIC_KEY > /opt/amnezia/awg/wireguard_server_public_key.key

WIREGUARD_PSK=$(wg genpsk)
echo $WIREGUARD_PSK > /opt/amnezia/awg/wireguard_psk.key

cat > /opt/amnezia/awg/wg0.conf <<EOF
[Interface]
PrivateKey = $WIREGUARD_SERVER_PRIVATE_KEY
Address = $AWG_SUBNET_IP/$WIREGUARD_SUBNET_CIDR
ListenPort = $AWG_SERVER_PORT
Jc = $JUNK_PACKET_COUNT
Jmin = $JUNK_PACKET_MIN_SIZE
Jmax = $JUNK_PACKET_MAX_SIZE
S1 = $INIT_PACKET_JUNK_SIZE
S2 = $RESPONSE_PACKET_JUNK_SIZE
H1 = $INIT_PACKET_MAGIC_HEADER
H2 = $RESPONSE_PACKET_MAGIC_HEADER
H3 = $UNDERLOAD_PACKET_MAGIC_HEADER
H4 = $TRANSPORT_PACKET_MAGIC_HEADER
EOF
