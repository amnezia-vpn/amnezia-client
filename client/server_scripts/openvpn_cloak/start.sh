#!/bin/bash

# This scripts copied from Amnezia client to Docker container to /opt/amnezia and launched every time container starts

echo "Container Startup start"

if [ ! -c /dev/net/tun ]; then mkdir -p /dev/net; mknod /dev/net/tun c 10 200; fi

# Allow traffic on the TUN interface.
iptables -A INPUT -i tun0 -j ACCEPT
iptables -A FORWARD -i tun0 -j ACCEPT
iptables -A OUTPUT -o tun0 -j ACCEPT

# Allow forwarding traffic only from the VPN.
iptables -A FORWARD -i tun0 -o eth0 -s $VPN_SUBNET_IP/$VPN_SUBNET_MASK_VAL -j ACCEPT
iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT

iptables -t nat -A POSTROUTING -s $VPN_SUBNET_IP/$VPN_SUBNET_MASK_VAL -o eth0 -j MASQUERADE

if [ -f /opt/amnezia/openvpn/ca.crt ]; then (openvpn --config /opt/amnezia/openvpn/server.conf --daemon); fi
if [ -f /opt/amnezia/cloak/ck-config.json ]; then (ck-server -c /opt/amnezia/cloak/ck-config.json &); fi

tail -f /dev/null
