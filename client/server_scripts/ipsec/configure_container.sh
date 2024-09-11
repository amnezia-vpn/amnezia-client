#!/bin/bash

export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

if [ ! -e /dev/ppp ]; then
cat <<'EOF'

Warning: /dev/ppp is missing, and IPsec/L2TP mode may not work. Please use
         IKEv2 (https://git.io/ikev2docker) or IPsec/XAuth mode to connect.
EOF
fi

NET_IFACE=$(route 2>/dev/null | grep -m 1 '^default' | grep -o '[^ ]*$')
[ -z "$NET_IFACE" ] && NET_IFACE=$(ip -4 route list 0/0 2>/dev/null | grep -m 1 -Po '(?<=dev )(\S+)')
[ -z "$NET_IFACE" ] && NET_IFACE=eth0


mkdir -p /opt/src
mkdir -p /opt/amnezia/ikev2/clients


# Create IPsec config
cat > /etc/ipsec.conf <<EOF
version 2.0

config setup
  virtual-private=%v4:10.0.0.0/8,%v4:192.168.0.0/16,%v4:172.16.0.0/12,%v4:!$IPSEC_VPN_L2TP_NET,%v4:!$IPSEC_VPN_XAUTH_NET
  uniqueids=no

conn shared
  left=%defaultroute
  leftid=$SERVER_IP_ADDRESS
  right=%any
  encapsulation=yes
  authby=secret
  pfs=no
  rekey=no
  keyingtries=5
  dpddelay=30
  dpdtimeout=120
  dpdaction=clear
  ikev2=never
  ike=aes256-sha2,aes128-sha2,aes256-sha1,aes128-sha1,aes256-sha2;modp1024,aes128-sha1;modp1024
  phase2alg=aes_gcm-null,aes128-sha1,aes256-sha1,aes256-sha2_512,aes128-sha2,aes256-sha2
  ikelifetime=24h
  salifetime=24h
  sha2-truncbug=$IPSEC_VPN_SHA2_TRUNCBUG

EOF

if [ "$IPSEC_VPN_DISABLE_L2TP" != "yes" ]; then
cat >> /etc/ipsec.conf <<'EOF'
conn l2tp-psk
  auto=add
  leftprotoport=17/1701
  rightprotoport=17/%any
  type=transport
  also=shared

EOF
fi

if [ "$IPSEC_VPN_DISABLE_XAUTH" != "yes" ]; then
cat >> /etc/ipsec.conf <<EOF
conn xauth-psk
  auto=add
  leftsubnet=0.0.0.0/0
  rightaddresspool=$IPSEC_VPN_XAUTH_POOL
  modecfgdns=$PRIMARY_DNS,$SECONDARY_DNS
  leftxauthserver=yes
  rightxauthclient=yes
  leftmodecfgserver=yes
  rightmodecfgclient=yes
  modecfgpull=yes
  cisco-unity=yes
  also=shared

EOF
fi

cat >> /etc/ipsec.conf <<'EOF'
include /etc/ipsec.d/*.conf
EOF

if uname -r | grep -qi 'coreos'; then
  sed -i '/phase2alg/s/,aes256-sha2_512//' /etc/ipsec.conf
fi

if grep -qs ike-frag /etc/ipsec.d/ikev2.conf; then
  sed -i 's/^[[:space:]]\+ike-frag=/  fragmentation=/' /etc/ipsec.d/ikev2.conf
fi


# Create xl2tpd config
cat > /etc/xl2tpd/xl2tpd.conf <<EOF
[global]
port = 1701

[lns default]
ip range = $IPSEC_VPN_L2TP_POOL
local ip = $IPSEC_VPN_L2TP_LOCAL
require chap = yes
refuse pap = yes
require authentication = yes
name = l2tpd
pppoptfile = /etc/ppp/options.xl2tpd
length bit = yes
EOF

# Set xl2tpd options
cat > /etc/ppp/options.xl2tpd <<EOF
+mschap-v2
ipcp-accept-local
ipcp-accept-remote
noccp
auth
mtu 1280
mru 1280
proxyarp
lcp-echo-failure 4
lcp-echo-interval 30
connect-delay 5000
ms-dns $PRIMARY_SERVER_DNS
ms-dns $SECONDARY_SERVER_DNS
EOF


# Update sysctl settings
syt='/sbin/sysctl -e -q -w'
$syt kernel.msgmnb=65536 2>/dev/null
$syt kernel.msgmax=65536 2>/dev/null
$syt net.ipv4.ip_forward=1 2>/dev/null
$syt net.ipv4.conf.all.accept_redirects=0 2>/dev/null
$syt net.ipv4.conf.all.send_redirects=0 2>/dev/null
$syt net.ipv4.conf.all.rp_filter=0 2>/dev/null
$syt net.ipv4.conf.default.accept_redirects=0 2>/dev/null
$syt net.ipv4.conf.default.send_redirects=0 2>/dev/null
$syt net.ipv4.conf.default.rp_filter=0 2>/dev/null
$syt "net.ipv4.conf.$NET_IFACE.send_redirects=0" 2>/dev/null
$syt "net.ipv4.conf.$NET_IFACE.rp_filter=0" 2>/dev/null

# Create IPTables rules
ipi='iptables -I INPUT'
ipf='iptables -I FORWARD'
ipp='iptables -t nat -I POSTROUTING'
res='RELATED,ESTABLISHED'
if ! iptables -t nat -C POSTROUTING -s "$IPSEC_VPN_L2TP_NET" -o "$NET_IFACE" -j MASQUERADE 2>/dev/null; then
  $ipi 1 -p udp --dport 1701 -m policy --dir in --pol none -j DROP
  $ipi 2 -m conntrack --ctstate INVALID -j DROP
  $ipi 3 -m conntrack --ctstate "$res" -j ACCEPT
  $ipi 4 -p udp -m multiport --dports 500,4500 -j ACCEPT
  $ipi 5 -p udp --dport 1701 -m policy --dir in --pol ipsec -j ACCEPT
  $ipi 6 -p udp --dport 1701 -j DROP
  $ipf 1 -m conntrack --ctstate INVALID -j DROP
  $ipf 2 -i "$NET_IFACE" -o ppp+ -m conntrack --ctstate "$res" -j ACCEPT
  $ipf 3 -i ppp+ -o "$NET_IFACE" -j ACCEPT
  $ipf 4 -i ppp+ -o ppp+ -j ACCEPT
  $ipf 5 -i "$NET_IFACE" -d "$IPSEC_VPN_XAUTH_NET" -m conntrack --ctstate "$res" -j ACCEPT
  $ipf 6 -s "$IPSEC_VPN_XAUTH_NET" -o "$NET_IFACE" -j ACCEPT
  $ipf 7 -s "$IPSEC_VPN_XAUTH_NET" -o ppp+ -j ACCEPT

  if [ "$IPSEC_VPN_VPN_ANDROID_MTU_FIX" = "yes" ]; then
  # Client-to-client traffic is allowed by default. To *disallow* such traffic,
  # uncomment below and restart the Docker container.
   $ipf 2 -i ppp+ -o ppp+ -s "$IPSEC_VPN_L2TP_NET" -d "$IPSEC_VPN_L2TP_NET" -j DROP
   $ipf 3 -s "$IPSEC_VPN_XAUTH_NET" -d "$IPSEC_VPN_XAUTH_NET" -j DROP
   $ipf 4 -i ppp+ -d "$IPSEC_VPN_XAUTH_NET" -j DROP
   $ipf 5 -s "$IPSEC_VPN_XAUTH_NET" -o ppp+ -j DROP
  fi

  iptables -A FORWARD -j DROP
  $ipp -s "$IPSEC_VPN_XAUTH_NET" -o "$NET_IFACE" -m policy --dir out --pol none -j MASQUERADE
  $ipp -s "$IPSEC_VPN_L2TP_NET" -o "$NET_IFACE" -j MASQUERADE
fi


if [ "$IPSEC_VPN_VPN_ANDROID_MTU_FIX" = "yes" ]; then
    echo "Applying fix for Android MTU/MSS issues..."
    iptables -t mangle -A FORWARD -m policy --pol ipsec --dir in \
      -p tcp -m tcp --tcp-flags SYN,RST SYN -m tcpmss --mss 1361:1536 \
      -j TCPMSS --set-mss 1360
    iptables -t mangle -A FORWARD -m policy --pol ipsec --dir out \
      -p tcp -m tcp --tcp-flags SYN,RST SYN -m tcpmss --mss 1361:1536 \
      -j TCPMSS --set-mss 1360

fi

# Update file attributes
touch /etc/ipsec.secrets /etc/ppp/chap-secrets /etc/ipsec.d/passwd
chmod 600 /etc/ipsec.secrets /etc/ppp/chap-secrets /etc/ipsec.d/passwd


echo
echo "Starting IPsec service..."
mkdir -p /run/pluto /var/run/pluto
rm -f /run/pluto/pluto.pid /var/run/pluto/pluto.pid

ipsec initnss >/dev/null
ipsec pluto --config /etc/ipsec.conf


# Start xl2tpd
mkdir -p /var/run/xl2tpd
rm -f /var/run/xl2tpd.pid
/usr/sbin/xl2tpd -c /etc/xl2tpd/xl2tpd.conf


################# IKEV2 ##################
if [ "$IPSEC_VPN_DISABLE_IKEV2" != "yes" ]; then
printf "y\n\nN\n" | certutil -z <(head -c 1024 /dev/urandom) \
  -S -x -n "IKEv2 VPN CA" \
  -s "O=IKEv2 VPN,CN=IKEv2 VPN CA" \
  -k rsa -g 3072 -v 120 \
  -d sql:/etc/ipsec.d -t "CT,," -2

certutil -z <(head -c 1024 /dev/urandom) \
   -S -c "IKEv2 VPN CA" -n "$SERVER_IP_ADDRESS" \
   -s "O=IKEv2 VPN,CN=$SERVER_IP_ADDRESS" \
   -k rsa -g 3072 -v 120 \
   -d sql:/etc/ipsec.d -t ",," \
   --keyUsage digitalSignature,keyEncipherment \
   --extKeyUsage serverAuth \
   --extSAN "ip:$SERVER_IP_ADDRESS,dns:$SERVER_IP_ADDRESS"

certutil -L -d sql:/etc/ipsec.d -n "IKEv2 VPN CA" -a | grep -v CERTIFICATE > /etc/ipsec.d/ca_cert_base64.p12

cat > /etc/ipsec.d/ikev2.conf <<EOF
conn ikev2-cp
  left=%defaultroute
  leftcert=$SERVER_IP_ADDRESS
  leftid=$SERVER_IP_ADDRESS
  leftsendcert=always
  leftsubnet=0.0.0.0/0
  leftrsasigkey=%cert
  right=%any
  rightid=%fromcert
  rightaddresspool=192.168.43.10-192.168.43.250
  rightca=%same
  rightrsasigkey=%cert
  narrowing=yes
  dpddelay=30
  dpdtimeout=120
  dpdaction=clear
  auto=add
  authby=rsa-sha1
  ikev2=insist
  rekey=no
  pfs=no
  ike=aes256-sha2,aes128-sha2,aes256-sha1,aes128-sha1
  phase2alg=aes_gcm-null,aes128-sha1,aes256-sha1,aes128-sha2,aes256-sha2
  ikelifetime=24h
  salifetime=24h
  encapsulation=yes
  modecfgdns=$PRIMARY_SERVER_DNS,$SECONDARY_SERVER_DNS
EOF

 ipsec auto --add ikev2-cp
else
 ipsec auto --delete ikev2-cp
fi # if [ "$IPSEC_VPN_DISABLE_IKEV2" != "yes" ]
