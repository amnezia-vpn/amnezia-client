#!/bin/bash

APP_NAME=FBLink
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

cleanup_launch_daemon() {
  local plist_path="$1"
  local service_label="$2"
  run_cmd launchctl bootout "system/${service_label}" || true
  run_cmd launchctl bootout system "$plist_path" || true
  run_cmd launchctl unload "$plist_path" || true
  run_cmd rm -f "$plist_path"
}

# Clean existing and legacy launch daemons to keep updates idempotent.
run_cmd killall -9 "FBLink-service" || true
run_cmd killall -9 "AmneziaVPN-service" || true
cleanup_launch_daemon "/Library/LaunchDaemons/FBLink.plist" "FBLink-service"
cleanup_launch_daemon "/Library/LaunchDaemons/AmneziaVPN.plist" "AmneziaVPN-service"

run_cmd sudo chmod -R a-w "$APP_PATH/"
run_cmd sudo chown -R root "$APP_PATH/"
run_cmd sudo chgrp -R wheel "$APP_PATH/"
run_cmd sudo chmod 755 "$APP_PATH/Contents/MacOS/${APP_NAME}-service" || true

log "Requesting ${APP_NAME} to quit gracefully"
run_cmd osascript -e 'tell application "FBLink" to quit' || true

PLIST_SOURCE="$APP_PATH/Contents/Resources/$PLIST_NAME"
if [ ! -f "$PLIST_SOURCE" ] && [ -f "$APP_PATH/Contents/MacOS/$PLIST_NAME" ]; then
  PLIST_SOURCE="$APP_PATH/Contents/MacOS/$PLIST_NAME"
fi
if [ -f "$PLIST_SOURCE" ]; then
  run_cmd cp -f "$PLIST_SOURCE" "$LAUNCH_DAEMONS_PLIST_NAME"
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

# Create a desktop alias for the active user.
ACTIVE_USER=$(stat -f%Su /dev/console 2>/dev/null || true)
if [ -n "$ACTIVE_USER" ] && [ "$ACTIVE_USER" != "root" ]; then
  ACTIVE_HOME=$(dscl . -read "/Users/$ACTIVE_USER" NFSHomeDirectory 2>/dev/null | awk '{print $2}')
  ACTIVE_DESKTOP="$ACTIVE_HOME/Desktop"
  if [ -d "$ACTIVE_DESKTOP" ]; then
    DESKTOP_LINK="$ACTIVE_DESKTOP/FBLink.app"
    run_cmd sudo -u "$ACTIVE_USER" ln -sfn "$APP_PATH" "$DESKTOP_LINK" || true
  fi
fi

log "Script finished"
