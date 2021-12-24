#!/bin/sh

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

## TCP
# NAT PREROUTING
iptables -t nat -N shadowsocks-nat
# Skip LoopBack, Reserved
iptables -t nat -A shadowsocks-nat -d 0/8 -j RETURN
iptables -t nat -A shadowsocks-nat -d 127/8 -j RETURN
iptables -t nat -A shadowsocks-nat -d 10/8 -j RETURN
iptables -t nat -A shadowsocks-nat -d 169.254/16 -j RETURN
iptables -t nat -A shadowsocks-nat -d 172.16/12 -j RETURN
iptables -t nat -A shadowsocks-nat -d 192.168/16 -j RETURN
iptables -t nat -A shadowsocks-nat -d 224/4 -j RETURN
iptables -t nat -A shadowsocks-nat -d 240/4 -j RETURN
# Bypass CN IPs
iptables -t nat -A shadowsocks-nat -m set --match-set cn dst -j RETURN
# Bypass sslocal's outbound data
iptables -t nat -A shadowsocks-nat -m mark --mark 0xff/0xff -j RETURN
# Redirect TCP to 60080
iptables -t nat -A shadowsocks-nat -p tcp -j REDIRECT --to-ports 60080
# Local TCP -> shadowsocks-nat
iptables -t nat -A OUTPUT -p tcp -j shadowsocks-nat
# LAN TCP -> shadowsocks-nat
iptables -t nat -A PREROUTING -p tcp -j shadowsocks-nat

## UDP
# Strategy Route
ip rule add fwmark 0x1 table 100
ip route add local 0.0.0.0/0 dev lo table 100

# TPROXY for LAN
iptables -t mangle -N shadowsocks-tproxy
# Skip LoopBack, Reserved
iptables -t mangle -A shadowsocks-tproxy -d 0/8 -j RETURN
iptables -t mangle -A shadowsocks-tproxy -d 127/8 -j RETURN
iptables -t mangle -A shadowsocks-tproxy -d 10/8 -j RETURN
iptables -t mangle -A shadowsocks-tproxy -d 169.254/16 -j RETURN
iptables -t mangle -A shadowsocks-tproxy -d 172.16/12 -j RETURN
iptables -t mangle -A shadowsocks-tproxy -d 192.168/16 -j RETURN
iptables -t mangle -A shadowsocks-tproxy -d 224/4 -j RETURN
iptables -t mangle -A shadowsocks-tproxy -d 240/4 -j RETURN
# Bypass CN IPs
iptables -t mangle -A shadowsocks-tproxy -m set --match-set cn dst -j RETURN
# Bypass sslocal's outbound data
iptables -t mangle -A shadowsocks-tproxy -m mark --mark 0xff/0xff -j RETURN
# TPROXY UDP to 60080
iptables -t mangle -A shadowsocks-tproxy -p udp -j TPROXY --on-ip 0.0.0.0 --on-port 60080 --tproxy-mark 0x01/0x01
#iptables -t mangle -A shadowsocks-tproxy -p tcp -j TPROXY --on-port 60080 --tproxy-mark 1/1


# TPROXY for Local
iptables -t mangle -N shadowsocks-tproxy-mark
# Skip LoopBack, Reserved
iptables -t mangle -A shadowsocks-tproxy-mark -d 127/8 -j RETURN
iptables -t mangle -A shadowsocks-tproxy-mark -d 10/8 -j RETURN
iptables -t mangle -A shadowsocks-tproxy-mark -d 169.254/16 -j RETURN
iptables -t mangle -A shadowsocks-tproxy-mark -d 172.16/12 -j RETURN
iptables -t mangle -A shadowsocks-tproxy-mark -d 192.168/16 -j RETURN
iptables -t mangle -A shadowsocks-tproxy-mark -d 224/4 -j RETURN
iptables -t mangle -A shadowsocks-tproxy-mark -d 240/4 -j RETURN
# Bypass CN IPs
iptables -t mangle -A shadowsocks-tproxy-mark -m set --match-set cn dst -j RETURN
# Bypass sslocal's outbound data
iptables -t mangle -A shadowsocks-tproxy-mark -m mark --mark 0xff/0xff -j RETURN
# Set MARK and reroute
iptables -t mangle -A shadowsocks-tproxy-mark -p udp -j MARK --set-xmark 0x01/0xffffffff
#iptables -t mangle -A shadowsocks-tproxy-mark -p tcp -j MARK --set-xmark 1

# Apply TPROXY to LAN
iptables -t mangle -A PREROUTING -p udp -j shadowsocks-tproxy
# Apply TPROXY for Local
iptables -t mangle -A OUTPUT -p udp -j shadowsocks-tproxy-mark

# DIVERT rules
# For optimizing TCP
# iptables -t mangle -N shadowsocks-divert
# iptables -t mangle -A shadowsocks-divert -j MARK --set-mark 1
# iptables -t mangle -A shadowsocks-divert -j ACCEPT
# iptables -t mangle -I PREROUTING -p tcp -m socket -j shadowsocks-divert
