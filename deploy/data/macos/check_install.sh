#!/bin/bash
if [ -d "/Applications/AmneziaVPN.app" ] || pgrep -x "AmneziaVPN-service" >/dev/null; then
  exit 1
fi
exit 0
