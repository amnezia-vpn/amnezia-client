#!/bin/bash

# This scripts copied from Amnezia client to Docker container to /opt/amnezia and launched every time container starts

echo "Container startup"
#ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up

# kill daemons in case of restart
wg-quick down /opt/amnezia/awg/wg0.conf

sysctl -w net.ipv6.conf.all.disable_ipv6=0 >/dev/null 2>&1 || true
sysctl -w net.ipv6.conf.default.disable_ipv6=0 >/dev/null 2>&1 || true
sysctl -w net.ipv6.conf.all.forwarding=1 >/dev/null 2>&1 || true
sysctl -w net.ipv6.conf.default.forwarding=1 >/dev/null 2>&1 || true

# wg-quick runs `set -e` with a teardown trap, so a failed `ip -6 address add`
# on an IPv6-disabled host aborts the whole tunnel (including IPv4). If the host
# has no usable IPv6, drop the v6 address so wg-quick won't try to add it.
if [ -f /proc/net/if_inet6 ] && [ "$(cat /proc/sys/net/ipv6/conf/all/disable_ipv6 2>/dev/null)" = "0" ]; then
    IPV6_OK=true
else
    IPV6_OK=false
    sed -i '/^Address[[:space:]]*=/s/,.*$//' /opt/amnezia/awg/wg0.conf
    echo "Host IPv6 unavailable, starting AmneziaWG IPv4-only"
fi

# start daemons if configured
if [ -f /opt/amnezia/awg/wg0.conf ]; then (wg-quick up /opt/amnezia/awg/wg0.conf); fi

# Allow traffic on the TUN interface.
iptables -A INPUT -i wg0 -j ACCEPT
iptables -A FORWARD -i wg0 -j ACCEPT
iptables -A OUTPUT -o wg0 -j ACCEPT

# Allow forwarding traffic only from the VPN.
iptables -A FORWARD -i wg0 -o eth0 -s $AWG_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -j ACCEPT
iptables -A FORWARD -i wg0 -o eth1 -s $AWG_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -j ACCEPT

iptables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT

iptables -t nat -A POSTROUTING -s $AWG_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -o eth0 -j MASQUERADE
iptables -t nat -A POSTROUTING -s $AWG_SUBNET_IP/$WIREGUARD_SUBNET_CIDR -o eth1 -j MASQUERADE

if [ "$IPV6_OK" = true ] && command -v ip6tables >/dev/null 2>&1; then
    ip6tables -A INPUT -i wg0 -j ACCEPT || true
    ip6tables -A FORWARD -i wg0 -j ACCEPT || true
    ip6tables -A OUTPUT -o wg0 -j ACCEPT || true

    ip6tables -A FORWARD -i wg0 -o eth0 -s $AWG_SUBNET_IPV6/$WIREGUARD_SUBNET_IPV6_CIDR -j ACCEPT || true
    ip6tables -A FORWARD -i wg0 -o eth1 -s $AWG_SUBNET_IPV6/$WIREGUARD_SUBNET_IPV6_CIDR -j ACCEPT || true
    ip6tables -A FORWARD -m state --state ESTABLISHED,RELATED -j ACCEPT || true

    ip6tables -t nat -A POSTROUTING -s $AWG_SUBNET_IPV6/$WIREGUARD_SUBNET_IPV6_CIDR -o eth0 -j MASQUERADE || true
    ip6tables -t nat -A POSTROUTING -s $AWG_SUBNET_IPV6/$WIREGUARD_SUBNET_IPV6_CIDR -o eth1 -j MASQUERADE || true
fi

tail -f /dev/null
