#!/bin/bash

APP_NAME=FBLinkVPN
APP_BIN_NAME=FBLink
ORG_NAME=FBLink
LOG_FOLDER=/var/log/$APP_NAME
LOG_FILE="$LOG_FOLDER/post-uninstall.log"
APP_PATH=/opt/$APP_NAME

if ! test -f $LOG_FILE; then
	touch $LOG_FILE
fi

date >> $LOG_FILE
echo "Uninstall Script started" >> $LOG_FILE
sudo killall -9 $APP_NAME 2>> $LOG_FILE

if command -v steamos-readonly &> /dev/null; then
	sudo steamos-readonly disable >> $LOG_FILE
	echo "steamos-readonly disabled" >> $LOG_FILE
fi

ls /opt/FBLinkVPN/client/lib/* | while IFS=: read -r dir; do
	sudo unlink $dir  >> $LOG_FILE
done

for SERVICE_UNIT in "$APP_NAME" "FBLink"; do
        sudo systemctl stop "$SERVICE_UNIT" >> $LOG_FILE 2>&1 || true
        sudo systemctl disable "$SERVICE_UNIT" >> $LOG_FILE 2>&1 || true
        sudo systemctl reset-failed "$SERVICE_UNIT" >> $LOG_FILE 2>&1 || true
done

sudo rm -f /etc/systemd/system/$APP_NAME.service >> $LOG_FILE 2>&1
sudo rm -f /etc/systemd/system/FBLink.service >> $LOG_FILE 2>&1
sudo rm -f /lib/systemd/system/$APP_NAME.service >> $LOG_FILE 2>&1
sudo rm -f /lib/systemd/system/FBLink.service >> $LOG_FILE 2>&1
sudo rm -f /usr/lib/systemd/system/$APP_NAME.service >> $LOG_FILE 2>&1
sudo rm -f /usr/lib/systemd/system/FBLink.service >> $LOG_FILE 2>&1
sudo systemctl daemon-reload >> $LOG_FILE 2>&1

if test -f $APP_PATH; then
        sudo rm -rf $APP_PATH >> $LOG_FILE
fi

if test -f /usr/sbin/$APP_NAME; then
        sudo rm -f /usr/sbin/$APP_NAME >> $LOG_FILE
fi

if test -f /usr/bin/$APP_NAME; then
        sudo rm -f /usr/bin/$APP_NAME >> $LOG_FILE
fi

if test -f /usr/local/bin/$APP_NAME; then
        sudo rm -f /usr/local/bin/$APP_NAME >> $LOG_FILE
fi

if test -f /usr/local/bin/$APP_BIN_NAME; then
        sudo rm -f /usr/local/bin/$APP_BIN_NAME >> $LOG_FILE
fi

if test -f /usr/local/sbin/$APP_NAME; then
        sudo rm -f /usr/local/sbin/$APP_NAME >> $LOG_FILE
fi

if test -f /usr/share/applications/$APP_NAME.desktop; then
	sudo rm -f /usr/share/applications/$APP_NAME.desktop >> $LOG_FILE

fi

if test -f /usr/share/pixmaps/FBLink.png; then
	sudo rm -f /usr/share/pixmaps/FBLink.png >> $LOG_FILE

fi

### Remove desktop launcher created for the user
TARGET_USER="$SUDO_USER"
if [ -z "$TARGET_USER" ] || [ "$TARGET_USER" = "root" ]; then
    TARGET_USER=$(logname 2>/dev/null || true)
fi
if [ -n "$TARGET_USER" ] && [ "$TARGET_USER" != "root" ]; then
    TARGET_HOME=$(getent passwd "$TARGET_USER" | cut -d: -f6)
    if test -f "$TARGET_HOME/Desktop/FBLink VPN.desktop"; then
        rm -f "$TARGET_HOME/Desktop/FBLink VPN.desktop" >> $LOG_FILE 2>&1
    fi
    if test -f "$TARGET_HOME/Desktop/$APP_NAME.desktop"; then
        rm -f "$TARGET_HOME/Desktop/$APP_NAME.desktop" >> $LOG_FILE 2>&1
    fi
fi

### Remove the service log file (keep post-uninstall.log)
if test -f "$LOG_FOLDER/FBLinkVPN-service.log"; then
    sudo rm -f "$LOG_FOLDER/FBLinkVPN-service.log" >> $LOG_FILE 2>&1
fi

### Remove user logs for current user only
TARGET_HOME="$HOME"
if [ -n "$SUDO_USER" ] && [ "$SUDO_USER" != "root" ]; then
    TARGET_HOME=$(getent passwd "$SUDO_USER" | cut -d: -f6)
fi
if test -d "$TARGET_HOME/.local/share/$ORG_NAME/$APP_NAME/log"; then
    rm -rf "$TARGET_HOME/.local/share/$ORG_NAME/$APP_NAME/log" >> $LOG_FILE 2>&1
fi

# Try to remove empty app and organization directories under user share
if rmdir "$TARGET_HOME/.local/share/$ORG_NAME/$APP_NAME" 2>/dev/null; then :; fi
if rmdir "$TARGET_HOME/.local/share/$ORG_NAME" 2>/dev/null; then :; fi

if command -v steamos-readonly &> /dev/null; then
	sudo steamos-readonly enable >> $LOG_FILE
	echo "steamos-readonly enabled" >> $LOG_FILE
fi

date >> $LOG_FILE
echo "Service after uninstall status:" >> $LOG_FILE
sudo systemctl status $APP_NAME >> $LOG_FILE
date >> $LOG_FILE
echo "Script finished" >> $LOG_FILE
