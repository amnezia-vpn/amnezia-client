#!/bin/bash

APP_NAME=AmneziaVPN
SERVICE_GROUP=amnvpn
PLIST_NAME=$APP_NAME.plist
LAUNCH_DAEMONS_PLIST_NAME=/Library/LaunchDaemons/$PLIST_NAME
LOG_FOLDER=/var/log/$APP_NAME
LOG_FILE="$LOG_FOLDER/post-install.log"
APP_PATH=/Applications/$APP_NAME.app

rm -rf "$LOG_FOLDER"
mkdir -p "$LOG_FOLDER"
echo "`date` Script started" > "$LOG_FILE"

log() {
  echo "`date` $*" >> "$LOG_FILE"
}

run_cmd() {
  log "CMD: $*"
  "$@" >> "$LOG_FILE" 2>&1
  local ec=$?
  log "EXIT: $ec"
  return $ec
}

# Handle new installations unpacked into localized folder
if [ -d "/Applications/${APP_NAME}.localized" ]; then
  log "Detected ${APP_NAME}.localized, migrating to standard path"
  run_cmd sudo rm -rf "$APP_PATH"
  run_cmd sudo mv "/Applications/${APP_NAME}.localized/${APP_NAME}.app" "$APP_PATH"
  run_cmd sudo rm -rf "/Applications/${APP_NAME}.localized"
fi

run_cmd launchctl bootout system "$LAUNCH_DAEMONS_PLIST_NAME" || run_cmd launchctl unload "$LAUNCH_DAEMONS_PLIST_NAME"
run_cmd rm -f "$LAUNCH_DAEMONS_PLIST_NAME"

# Add separate group for xray filtering
if dscl . -read "/Groups/$SERVICE_GROUP" >/dev/null 2>&1; then
  log "Group $SERVICE_GROUP already exists"
  return 0
else
  local next_gid
  next_gid=$(dscl . -list /Groups PrimaryGroupID 2>/dev/null | awk '{print $2}' | sort -n | awk '$1>=500{g=$1} END{print (g?g+1:501)}')
  run_cmd dscl . -create "/Groups/$SERVICE_GROUP"
  run_cmd dscl . -create "/Groups/$SERVICE_GROUP" PrimaryGroupID "$next_gid"
  run_cmd dscl . -create "/Groups/$SERVICE_GROUP" RealName "Amnezia VPN Service Group"
fi

run_cmd sudo chmod -R a-w "$APP_PATH/"
run_cmd sudo chown -R root "$APP_PATH/"
run_cmd sudo chgrp -R wheel "$APP_PATH/"

log "Requesting ${APP_NAME} to quit gracefully"
run_cmd osascript -e 'tell application "AmneziaVPN" to quit' || true

PLIST_SOURCE="$APP_PATH/Contents/Resources/$PLIST_NAME"
if [ -f "$PLIST_SOURCE" ]; then
  run_cmd mv -f "$PLIST_SOURCE" "$LAUNCH_DAEMONS_PLIST_NAME"
else
  log "ERROR: service plist not found at $PLIST_SOURCE"
fi

run_cmd chown root:wheel "$LAUNCH_DAEMONS_PLIST_NAME"
run_cmd chmod 644 "$LAUNCH_DAEMONS_PLIST_NAME"
run_cmd launchctl bootstrap system "$LAUNCH_DAEMONS_PLIST_NAME" || run_cmd launchctl load "$LAUNCH_DAEMONS_PLIST_NAME"
run_cmd launchctl enable "system/$APP_NAME-service" || true
run_cmd launchctl kickstart -k "system/$APP_NAME-service" || true
run_cmd launchctl print "system/$APP_NAME-service" || true
log "Launching ${APP_NAME} application"
run_cmd open -a "$APP_PATH" || true

log "Script finished"
