#!/bin/sh
# CryptPad startup script
# The upstream Docker image starts the app on port 3000 automatically.
# Ensure nginx reverse proxy for HTTPS is running.

if pgrep -x nginx >/dev/null 2>&1; then
  nginx -s reload || true
else
  nginx || true
fi

echo "CryptPad service starts on 3000; HTTPS served by nginx on 443."
exit 0
