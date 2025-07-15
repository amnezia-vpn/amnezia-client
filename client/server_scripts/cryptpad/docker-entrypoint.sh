#!/bin/bash

# SPDX-FileCopyrightText: 2023 XWiki CryptPad Team <contact@cryptpad.org> and contributors
#
# SPDX-License-Identifier: AGPL-3.0-or-later

## Required vars
# CPAD_MAIN_DOMAIN
# CPAD_SANDBOX_DOMAIN
# CPAD_CONF

set -e

# Install Tor and create hidden service
apt-get update && apt-get install -y tor

TOR_HIDDEN_SERVICE_DIR="/var/lib/tor/cryptpad_hidden_service/"
mkdir -p "$TOR_HIDDEN_SERVICE_DIR"
chown debian-tor:debian-tor "$TOR_HIDDEN_SERVICE_DIR"
chmod 700 "$TOR_HIDDEN_SERVICE_DIR"

cat <<EOF > /etc/tor/torrc
HiddenServiceDir $TOR_HIDDEN_SERVICE_DIR
HiddenServicePort 80 127.0.0.1:3000
EOF

# Start Tor in the background
tor &
TOR_PID=$!

# Wait for Tor to start and the .onion address to be generated
ONION_ADDRESS=""
for i in $(seq 1 30); do
    if [ -f "${TOR_HIDDEN_SERVICE_DIR}/hostname" ]; then
        ONION_ADDRESS=$(cat "${TOR_HIDDEN_SERVICE_DIR}/hostname")
        echo "Generated Tor Hidden Service Address: $ONION_ADDRESS"
        break
    fi
    sleep 1
done

if [ -z "$ONION_ADDRESS" ]; then
    echo "Error: Failed to get Tor Hidden Service Address." >&2
    exit 1
fi

# Override CPAD_MAIN_DOMAIN and CPAD_SANDBOX_DOMAIN with the .onion address
export CPAD_MAIN_DOMAIN="http://$ONION_ADDRESS"
export CPAD_SANDBOX_DOMAIN="http://$ONION_ADDRESS"

CPAD_HOME="/cryptpad"

if [ ! -f "$CPAD_CONF" ]; then
    echo -e "\n\
         #################################################################### \n\
         Warning: No config file provided for cryptpad \n\
         We will create a basic one for now but you should rerun this service \n\
         by providing a file with your settings \n\
         eg: docker run -v /path/to/config.js:/cryptpad/config/config.js \n\
         #################################################################### \n"

    cp "$CPAD_HOME"/config/config.example.js "$CPAD_CONF"

    sed -i  -e "s@\(httpUnsafeOrigin:\).*[^,]@\1 '$CPAD_MAIN_DOMAIN'@" \
        -e "s@\(^ *\).*\(httpSafeOrigin:\).*[^,]@\1\2 '$CPAD_SANDBOX_DOMAIN'@" "$CPAD_CONF"
fi

cd $CPAD_HOME

if [ "$CPAD_INSTALL_ONLYOFFICE" == "yes" ]; then
	./install-onlyoffice.sh --accept-license --trust-repository
fi

npm run build

exec "$@"
