cat > /opt/amnezia/openvpn/server.conf <<EOF
port $OPENVPN_PORT
proto $OPENVPN_TRANSPORT_PROTO
dev tun
ca /opt/amnezia/openvpn/ca.crt
cert /opt/amnezia/openvpn/AmneziaReq.crt
key /opt/amnezia/openvpn/AmneziaReq.key
dh /opt/amnezia/openvpn/dh.pem
server $OPENVPN_SUBNET_IP $OPENVPN_SUBNET_MASK
ifconfig-pool-persist ipp.txt
duplicate-cn
keepalive 10 120
$OPENVPN_NCP_DISABLE
cipher $OPENVPN_CIPHER
data-ciphers $OPENVPN_CIPHER
auth $OPENVPN_HASH
user nobody
group nobody
persist-key
persist-tun
status openvpn-status.log
verb 1
tls-server
tls-version-min 1.2
$OPENVPN_TLS_AUTH
$OPENVPN_ADDITIONAL_SERVER_CONFIG
EOF

# V2RAY_SERVER_PORT port for v2ray  listening, for example 10086.
# V2RAY_VMESS_CLIENT_UUID client's id and secret as UUID.
# UUID is 32 hexadecimal digits /([0-9a-f]-?){32}/ (128 bit value).

mkdir -p /opt/amnezia/v2ray
cd /opt/amnezia/v2ray
V2RAY_VMESS_CLIENT_UUID="b831381d-6324-4d53-ad4f-8cda48b30811" # $(openssl rand -base64 32 | tr "=" "A" | tr "+" "A" | tr "/" "A")
echo $V2RAY_VMESS_CLIENT_UUID > /opt/amnezia/v2ray/v2ray.key

cat < /opt/amnezia/v2ray/v2ray-server.json <<EOF
{
  "log": {
    "loglevel": "None"
  },
  "inbounds": [
    {
      "port": $V2RAY_SERVER_PORT,
      "protocol": "vmess",
      "settings": {
        "clients": [
          {
            "id": "$V2RAY_VMESS_CLIENT_UUID",
            "level": 1,
            "alterId": 64
          }
        ]
      }
    }
  ],
  "outbounds": [
    {
      "protocol": "freedom",
      "settings": {}
    }
  ],
  "policy": {
    "levels": {
      "0": {"uplinkOnly": 0}
    }
  }
}
EOF