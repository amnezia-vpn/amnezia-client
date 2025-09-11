#!/bin/bash
if [ -d "/Applications/AmneziaVPN.app" ] || pgrep -x "AmneziaVPN-service" >/dev/null; then
  exit 0
fi
exit 1
