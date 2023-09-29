#!/bin/bash

# This scripts copied from Amnezia client to Docker container to /opt/amnezia and launched every time container starts

echo "Container startup"
#ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up

# kill daemons in case of restart
# wg-quick down /opt/amnezia/amneziawireguard/wg0.conf

/usr/bin/amnezia-wg wg0 && /usr/bin/wg setconf wg0 /opt/amnezia/amneziawireguard/wg0.conf && ip address add dev wg0 10.8.1.1/24 && ip link set up dev wg0
# # # start daemons if configured
# # if [ -f /opt/amnezia/amneziawireguard/wg0.conf ]; then (wg-quick up /opt/amnezia/amneziawireguard/wg0.conf); fi

# Allow traffic on the TUN interface.
iptables -A INPUT -i wg0 -j ACCEPT
iptables -A FORWARD -i wg0 -j ACCEPT
iptables -A OUTPUT -o wg0 -j ACCEPT

# Allow forwarding traffic only from the VPN.
iptables -A FORWARD -i wg0 -o eth0 -s $WIREGUARD_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -j ACCEPT
iptables -A FORWARD -i wg0 -o eth1 -s $WIREGUARD_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -j ACCEPT

iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT

iptables -t nat -A POSTROUTING -s $WIREGUARD_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -o eth0 -j MASQUERADE
iptables -t nat -A POSTROUTING -s $WIREGUARD_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -o eth1 -j MASQUERADE

tail -f /dev/null
