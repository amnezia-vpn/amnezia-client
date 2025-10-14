#!/bin/bash
set -e

# Configure CryptPad to allow external HTTP access by setting httpUnsafeOrigin
# Uses variables substituted by the client before execution: $SERVER_IP_ADDRESS, $CRYPTPAD_PORT

CONFIG_DIR="/cryptpad/config"
CONFIG_FILE="$CONFIG_DIR/config.js"

mkdir -p "$CONFIG_DIR"

cat > "$CONFIG_FILE" <<EOF
module.exports = {
  // Public origin for HTTP access (non-HTTPS). Must include trailing slash.
  httpUnsafeOrigin: 'http://$SERVER_IP_ADDRESS:$CRYPTPAD_PORT/'
};
EOF

echo "CryptPad config written to $CONFIG_FILE"

# Restart container process to apply config (Docker will auto-restart due to --restart always)
echo "Restarting container to apply CryptPad config..."
kill 1
