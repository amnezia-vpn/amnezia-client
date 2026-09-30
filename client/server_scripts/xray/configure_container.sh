cd /opt/amnezia/xray
XRAY_CLIENT_ID=$(xray uuid) && echo $XRAY_CLIENT_ID > /opt/amnezia/xray/xray_uuid.key
XRAY_SHORT_ID=$(openssl rand -hex 8) && echo $XRAY_SHORT_ID > /opt/amnezia/xray/xray_short_id.key

# Parse x25519 keypair by label (v26.7 output has an extra Hash32 line; line-index parsing breaks).
KEYPAIR=$(xray x25519)
XRAY_PRIVATE_KEY=$(printf '%s\n' "$KEYPAIR" | sed -n 's/.*[Pp]rivate[ ]*[Kk]ey:[[:space:]]*//p' | head -1)
XRAY_PUBLIC_KEY=$(printf '%s\n' "$KEYPAIR" | sed -n 's/.*(PublicKey):[[:space:]]*//p' | head -1)
[ -z "$XRAY_PUBLIC_KEY" ] && XRAY_PUBLIC_KEY=$(printf '%s\n' "$KEYPAIR" | sed -n 's/.*[Pp]ublic[ ]*[Kk]ey:[[:space:]]*//p' | head -1)

XRAY_PRIVATE_KEY=$(echo $XRAY_PRIVATE_KEY | tr -d ' ')
XRAY_PUBLIC_KEY=$(echo $XRAY_PUBLIC_KEY | tr -d ' ')


echo $XRAY_PUBLIC_KEY > /opt/amnezia/xray/xray_public.key
echo $XRAY_PRIVATE_KEY > /opt/amnezia/xray/xray_private.key

# server.json is written by the client (writeServerConfigForSetup); this script only makes keys.
