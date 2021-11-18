{
  "uuid": "$UUID",
  "name": "IKEv2 VPN ($SERVER_ADDR)",
  "type": "ikev2-cert",
  "remote": {
    "addr": "$SERVER_ADDR"
  },
  "local": {
    "p12": "$P12_BASE64",
    "rsa-pss": "true"
  },
  "ike-proposal": "aes256-sha256-modp2048",
  "esp-proposal": "aes128gcm16"
}