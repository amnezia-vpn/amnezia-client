cd /opt/amnezia/xray
XRAY_CLIENT_ID=$(xray uuid) && echo $XRAY_CLIENT_ID > /opt/amnezia/xray/xray_uuid.key
XRAY_SHORT_ID=$(openssl rand -hex 8) && echo $XRAY_SHORT_ID > /opt/amnezia/xray/xray_short_id.key

KEYPAIR=$(xray x25519)
LINE_NUM=1
while IFS= read -r line; do
   if [[ $LINE_NUM -gt 1 ]]
      then
           IFS=":" read FIST XRAY_PUBLIC_KEY <<< "$line"
      else
      	   LINE_NUM=$((LINE_NUM + 1))
           IFS=":" read FIST XRAY_PRIVATE_KEY <<< "$line"
      fi
done <<< "$KEYPAIR"

XRAY_PRIVATE_KEY=$(echo $XRAY_PRIVATE_KEY | tr -d ' ')
XRAY_PUBLIC_KEY=$(echo $XRAY_PUBLIC_KEY | tr -d ' ')


echo $XRAY_PUBLIC_KEY > /opt/amnezia/xray/xray_public.key
echo $XRAY_PRIVATE_KEY > /opt/amnezia/xray/xray_private.key


cat > /opt/amnezia/xray/server.json <<EOF
{
    "log": {
        "loglevel": "error"
    },
    "inbounds": [
        {
            "port": 443,
            "protocol": "vless",
            "settings": {
                "clients": [
                    {
                        "id": "$XRAY_CLIENT_ID",
                        "flow": "xtls-rprx-vision"
                    }
                ],
                "decryption": "none"
            },
            "streamSettings": {
                "network": "tcp",
                "security": "reality",
                "realitySettings": {
                    "dest": "$XRAY_SITE_NAME:443",
                    "serverNames": [
                        "$XRAY_SITE_NAME"
                    ],
                    "privateKey": "$XRAY_PRIVATE_KEY",
                    "shortIds": [
                        "$XRAY_SHORT_ID"
                    ]
                }
            }
        }
    ],
    "outbounds": [
        {
            "protocol": "freedom"
        }
    ]
}
EOF


