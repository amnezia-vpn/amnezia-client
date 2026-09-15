#!/bin/bash

APP_NAME=AmneziaVPN
PLIST_NAME=$APP_NAME.plist
LAUNCH_DAEMONS_PLIST_NAME="/Library/LaunchDaemons/$PLIST_NAME"
APP_PATH="/Applications/$APP_NAME.app"
USER_APP_SUPPORT="$HOME/Library/Application Support/$APP_NAME"
SYSTEM_APP_SUPPORT="/Library/Application Support/$APP_NAME"
LOG_FOLDER="/var/log/$APP_NAME"
CACHES_FOLDER="$HOME/Library/Caches/$APP_NAME"
SERVICE_GROUP="amnvpn"

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
    sudo launchctl bootout system "$LAUNCH_DAEMONS_PLIST_NAME" || sudo launchctl unload "$LAUNCH_DAEMONS_PLIST_NAME"
fi
sudo rm -f "$LAUNCH_DAEMONS_PLIST_NAME"

# ---------------- Split-tunnel system extension ------------------------------
# Must run while the bundle is still on disk: systemextensionsctl resolves the
# extension through its host app.
ST_BUNDLE_ID="org.amnezia.AmneziaVPN.network-extension"
ST_TEAM_ID="X7UJ388FXK"

if command -v systemextensionsctl >/dev/null 2>&1; then
    if systemextensionsctl list 2>/dev/null | grep -q "$ST_BUNDLE_ID"; then
        echo "Removing split-tunnel system extension $ST_BUNDLE_ID"
        if ! sudo systemextensionsctl uninstall "$ST_TEAM_ID" "$ST_BUNDLE_ID"; then
            echo "Could not remove the system extension automatically."
            echo "Remove it manually in System Settings > General > Login Items & Extensions > Network Extensions."
        fi
    else
        echo "No split-tunnel system extension registered"
    fi
fi

# The "AmneziaVPN Split Tunnel" entry in System Settings > Network belongs to
# the NetworkExtension preferences and can only be removed by the app itself
# (it does so when app split tunneling is switched off). If one is left over,
# remove it from System Settings > Network.

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

# Remove amnvpn group if it's not referenced by users
if dscl . -read "/Groups/$SERVICE_GROUP" >/dev/null 2>&1; then
    group_gid=$(dscl . -read "/Groups/$SERVICE_GROUP" PrimaryGroupID 2>/dev/null | awk '{print $2}')
    users_with_primary_gid=""
    if [ -n "$group_gid" ]; then
        users_with_primary_gid=$(dscl . -list /Users PrimaryGroupID 2>/dev/null | awk -v gid="$group_gid" '$2 == gid {print $1}')
    fi

    if [ -z "$users_with_primary_gid" ]; then
        echo "Removing group $SERVICE_GROUP"
        sudo dscl . -delete "/Groups/$SERVICE_GROUP" || true
    else
        echo "Keeping group $SERVICE_GROUP (still used by users): $users_with_primary_gid"
    fi
fi

# -----------------------------------------------------------
