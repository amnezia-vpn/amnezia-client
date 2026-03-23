#!/bin/bash
if [ -d "/Applications/FBLink.app" ] || pgrep -x "FBLink-service" >/dev/null; then
  exit 1
fi
exit 0
