#!/bin/bash

APP_NAME=AmneziaVPN
PLIST_NAME=$APP_NAME.plist
LAUNCH_DAEMONS_PLIST_NAME=/Library/LaunchDaemons/$PLIST_NAME
LOG_FOLDER=/var/log/$APP_NAME
LOG_FILE="$LOG_FOLDER/post-install.log"
APP_PATH=/Applications/$APP_NAME.app

# Handle new installations unpacked into localized folder
if [ -d "/Applications/${APP_NAME}.localized" ]; then
  echo "`date` Detected ${APP_NAME}.localized, migrating to standard path" >> $LOG_FILE
  sudo rm -rf "$APP_PATH"
  sudo mv "/Applications/${APP_NAME}.localized/${APP_NAME}.app" "$APP_PATH"
  sudo rm -rf "/Applications/${APP_NAME}.localized"
fi

if launchctl list "$APP_NAME-service" &> /dev/null; then
    launchctl unload "$LAUNCH_DAEMONS_PLIST_NAME"
    rm -f "$LAUNCH_DAEMONS_PLIST_NAME"
fi

sudo chmod -R a-w "$APP_PATH/"
sudo chown -R root "$APP_PATH/"
sudo chgrp -R wheel "$APP_PATH/"

rm -rf	$LOG_FOLDER
mkdir -p $LOG_FOLDER

echo "`date` Script started" > $LOG_FILE

echo "Requesting ${APP_NAME} to quit gracefully" >> "$LOG_FILE"
osascript -e 'tell application "AmneziaVPN" to quit'

PLIST_SOURCE="$APP_PATH/Contents/Resources/$PLIST_NAME"
if [ -f "$PLIST_SOURCE" ]; then
  mv -f "$PLIST_SOURCE" "$LAUNCH_DAEMONS_PLIST_NAME" 2>> $LOG_FILE
else
  echo "`date` ERROR: service plist not found at $PLIST_SOURCE" >> $LOG_FILE
fi

chown root:wheel "$LAUNCH_DAEMONS_PLIST_NAME"
launchctl load "$LAUNCH_DAEMONS_PLIST_NAME"
echo "`date` Launching ${APP_NAME} application" >> $LOG_FILE
open -a "$APP_PATH" 2>> $LOG_FILE || true

echo "`date` Service status: $?" >> $LOG_FILE
echo "`date` Script finished" >> $LOG_FILE
