#!/bin/sh
sysctl net.ipv6.conf.all.disable_ipv6=0 || true
/yggdrasil -useconffile /config/yggdrasil.conf