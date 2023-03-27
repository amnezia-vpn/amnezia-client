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

# V2RAY_TROJAN_PORT port for v2ray listening, for example 10086.
# V2RAY_TROJAN_CLIENT_PASSWORD client's password.

V2RAY_TROJAN_ROOT=/opt/amnezia/v2ray_trojan 
mkdir -p $V2RAY_TROJAN_ROOT
cd $V2RAY_TROJAN_ROOT
V2RAY_TROJAN_CLIENT_PASSWORD=$(tr -dc 'A-Za-z0-9!"#$%&'\''()*+,-./:;<=>?@[\]^_`{|}~' </dev/random | head -c 20)
echo $V2RAY_TROJAN_CLIENT_PASSWORD > $V2RAY_TROJAN_ROOT/v2ray_trojan.key

cat > $V2RAY_TROJAN_ROOT/v2ray-server.json <<EOF
{
  "log": {
    "loglevel": "None"
  },
  "inbounds": [
    {
      "port": $V2RAY_TROJAN_PORT,
      "protocol": "trojan",
      "settings": {
        "users": [
          "$V2RAY_TROJAN_CLIENT_PASSWORD"
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
