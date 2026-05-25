#!/bin/sh

echo "Container startup (Telemt)"

if [ ! -f /data/config.toml ]; then
    echo "ERROR: /data/config.toml not found — run configure_container first"
    tail -f /dev/null
    exit 1
fi

mkdir -p /data/tlsfront
exec /usr/local/bin/telemt /data/config.toml
