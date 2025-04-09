#!/bin/bash

APP_NAME=AmneziaVPN
PLIST_NAME=$APP_NAME.plist
LAUNCH_DAEMONS_PLIST_NAME="/Library/LaunchDaemons/$PLIST_NAME"
APP_PATH="/Applications/$APP_NAME.app"
USER_APP_SUPPORT="$HOME/Library/Application Support/$APP_NAME"
SYSTEM_APP_SUPPORT="/Library/Application Support/$APP_NAME"
LOG_FOLDER="/var/log/$APP_NAME"
CACHES_FOLDER="$HOME/Library/Caches/$APP_NAME"

# Stop the running service if it exists
if pgrep -x "${APP_NAME}-service" > /dev/null; then
    sudo killall -9 "${APP_NAME}-service"
fi

# Unload the service if loaded and remove its plist file regardless
if launchctl list "${APP_NAME}-service" &> /dev/null; then
    sudo launchctl unload "$LAUNCH_DAEMONS_PLIST_NAME"
fi
sudo rm -f "$LAUNCH_DAEMONS_PLIST_NAME"

# Remove the entire application bundle
sudo rm -rf "$APP_PATH"

# Remove Application Support folders (user and system, if they exist)
rm -rf "$USER_APP_SUPPORT"
sudo rm -rf "$SYSTEM_APP_SUPPORT"

# Remove the log folder
sudo rm -rf "$LOG_FOLDER"

# Remove any caches left behind
rm -rf "$CACHES_FOLDER"
