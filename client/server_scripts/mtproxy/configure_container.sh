#!/bin/sh
set -eu

secret_file="/data/secret"
tries=0

while [ $tries -lt 10 ] && [ ! -f "$secret_file" ]; do
    tries=$((tries + 1))
    sleep 1
done

if [ -f "$secret_file" ]; then
    secret="$(tr -d '\r\n ' < "$secret_file" | cut -c1-32 | tr 'A-F' 'a-f')"
    if [ -n "$secret" ]; then
        echo "MTPROXY_SECRET=$secret"
    fi
else
    echo "WARNING: secret file not found after 10 seconds"
fi

if [ "${TAG:-}" != "" ]; then
    tag="$(printf "%s" "$TAG" | tr -d '\r\n ' | cut -c1-32 | tr 'A-F' 'a-f')"
    if [ -n "$tag" ]; then
        echo "MTPROXY_TAG=$tag"
    fi
fi
