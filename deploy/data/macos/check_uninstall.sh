#!/bin/bash
if [ -d "/Applications/FBLink.app" ] || pgrep -x "FBLink-service" >/dev/null; then
  exit 0
fi
exit 1
