mkdir -p /opt/amnezia/awg
cd /opt/amnezia/awg
WIREGUARD_SERVER_PRIVATE_KEY=$(awg genkey)
echo $WIREGUARD_SERVER_PRIVATE_KEY > /opt/amnezia/awg/wireguard_server_private_key.key

WIREGUARD_SERVER_PUBLIC_KEY=$(echo $WIREGUARD_SERVER_PRIVATE_KEY | awg pubkey)
echo $WIREGUARD_SERVER_PUBLIC_KEY > /opt/amnezia/awg/wireguard_server_public_key.key

WIREGUARD_PSK=$(awg genpsk)
echo $WIREGUARD_PSK > /opt/amnezia/awg/wireguard_psk.key

cat > /opt/amnezia/awg/awg0.conf <<EOF
[Interface]
PrivateKey = $WIREGUARD_SERVER_PRIVATE_KEY
Address = $AWG_SUBNET_IP/$WIREGUARD_SUBNET_CIDR
ListenPort = $AWG_SERVER_PORT
Jc = $JUNK_PACKET_COUNT
Jmin = $JUNK_PACKET_MIN_SIZE
Jmax = $JUNK_PACKET_MAX_SIZE
S1 = $INIT_PACKET_JUNK_SIZE
S2 = $RESPONSE_PACKET_JUNK_SIZE
S3 = $COOKIE_REPLY_PACKET_JUNK_SIZE
S4 = $TRANSPORT_PACKET_JUNK_SIZE
H1 = $INIT_PACKET_MAGIC_HEADER
H2 = $RESPONSE_PACKET_MAGIC_HEADER
H3 = $UNDERLOAD_PACKET_MAGIC_HEADER
H4 = $TRANSPORT_PACKET_MAGIC_HEADER
HeaderProtectionKey = $HEADER_PROTECTION_KEY
ContentPaddingAddition = $CONTENT_PADDING_ADDITION
RekeyAfterTime = $REKEY_AFTER_TIME
RekeyTimeout = $REKEY_TIMEOUT
RejectAfterTime = $REJECT_AFTER_TIME
KeepaliveTimeout = $KEEPALIVE_TIMEOUT
MaxHandshakeAttempts = $MAX_HANDSHAKE_ATTEMPTS
RandomTrailers = $RANDOM_TRAILERS
DisableCookies = $DISABLE_COOKIES
# I1 = $SPECIAL_JUNK_1
# I2 = $SPECIAL_JUNK_2
# I3 = $SPECIAL_JUNK_3
# I4 = $SPECIAL_JUNK_4
# I5 = $SPECIAL_JUNK_5
EOF

# Every AWG parameter is optional - drop the lines whose value came out empty
sed -i '/^[^=]*= *$/d' /opt/amnezia/awg/awg0.conf
