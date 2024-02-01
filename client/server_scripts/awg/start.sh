#!/bin/bash

# This scripts copied from Amnezia client to Docker container to /opt/amnezia and launched every time container starts

echo "Container startup"
#ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up

# kill daemons in case of restart
wg-quick down /opt/amnezia/awg/wg0.conf

# start daemons if configured
if [ -f /opt/amnezia/awg/wg0.conf ]; then (wg-quick up /opt/amnezia/awg/wg0.conf); fi

# Allow traffic on the TUN interface.
iptables -A INPUT -i wg0 -j ACCEPT
iptables -A OUTPUT -o wg0 -j ACCEPT

# Reject forwarding traffic between users via wg0 interface
iptables -A FORWARD -s 10.8.1.0/24 -d 10.8.1.0/24 -j REJECT

iptables -t nat -A POSTROUTING -s 10.8.1.0/24 -o eth0 -j MASQUERADE
iptables -t nat -A POSTROUTING -s 10.8.1.0/24 -o eth1 -j MASQUERADE

tail -f /dev/null
