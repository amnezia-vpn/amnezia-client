#!/bin/bash

APP_NAME=AmneziaVPN
PLIST_NAME=$APP_NAME.plist
LAUNCH_DAEMONS_PLIST_NAME="/Library/LaunchDaemons/$PLIST_NAME"
APP_PATH="/Applications/$APP_NAME.app"
USER_APP_SUPPORT="$HOME/Library/Application Support/$APP_NAME"
SYSTEM_APP_SUPPORT="/Library/Application Support/$APP_NAME"
LOG_FOLDER="/var/log/$APP_NAME"
CACHES_FOLDER="$HOME/Library/Caches/$APP_NAME"

# Attempt to quit the GUI application if it's currently running
if pgrep -x "$APP_NAME" > /dev/null; then
    echo "Quitting $APP_NAME..."
    osascript -e 'tell application "'"$APP_NAME"'" to quit' || true
    # Wait up to 10 seconds for the app to terminate gracefully
    for i in {1..10}; do
        if ! pgrep -x "$APP_NAME" > /dev/null; then
            break
        fi
        sleep 1
    done
fi

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

# Remove PF data directory created by firewall helper, if present
sudo rm -rf "/Library/Application Support/${APP_NAME}/pf"

# ---------------- PF firewall cleanup ----------------------
# Rules are loaded under the anchor "amn" (see macosfirewall.cpp)
# Flush only that anchor to avoid destroying user/system rules.

PF_ANCHOR="amn"

### Flush all PF rules, NATs, and tables under our anchor and sub-anchors ###
anchors=$(sudo pfctl -s Anchors 2>/dev/null | awk '/^'"${PF_ANCHOR}"'/ {sub(/\*$/, "", $1); print $1}')
for anc in $anchors; do
    echo "Flushing PF anchor $anc"
    sudo pfctl -a "$anc" -F all 2>/dev/null || true
    # flush tables under this anchor
    tables=$(sudo pfctl -s Tables 2>/dev/null | awk '/^'"$anc"'/ {print}')
    for tbl in $tables; do
        echo "Killing PF table $tbl"
        sudo pfctl -t "$tbl" -T kill 2>/dev/null || true
    done
done

### Reload default PF config to restore system rules ###
if [ -f /etc/pf.conf ]; then
    echo "Restoring system PF config"
    sudo pfctl -f /etc/pf.conf 2>/dev/null || true
fi

### Disable PF if no rules remain ###
if sudo pfctl -s info 2>/dev/null | grep -q '^Status: Enabled' && \
   ! sudo pfctl -sr 2>/dev/null | grep -q .; then
    echo "Disabling PF"
    sudo pfctl -d 2>/dev/null || true
fi

# -----------------------------------------------------------
