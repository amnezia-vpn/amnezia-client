#!/bin/bash

APP_NAME=FBLinkVPN
APP_BIN_NAME=FBLink
LOG_FOLDER=/var/log/$APP_NAME
LOG_FILE="$LOG_FOLDER/post-install.log"
APP_PATH=/opt/$APP_NAME

sudo() {
        if [ "${EUID:-$(id -u)}" -eq 0 ]; then
                "$@"
        else
                command sudo -n "$@"
        fi
}

if ! test -d "$LOG_FOLDER"; then
        sudo mkdir $LOG_FOLDER
echo "FBLinkVPN log dir created at /var/log/"
fi

if ! test -f "$LOG_FILE"; then
        touch "$LOG_FILE"
        echo "FBLinkVPN log file created at /var/log/FBLinkVPN/post-install.log"
fi

date > $LOG_FILE
echo "Script started" >> $LOG_FILE
sudo killall -9 $APP_NAME 2>> $LOG_FILE

if command -v steamos-readonly &> /dev/null; then
        sudo steamos-readonly disable >> $LOG_FILE
        echo "steamos-readonly disabled" >> $LOG_FILE
fi

echo "Skipping package-manager dependency installation inside installer to avoid blocking GUI flow." >> $LOG_FILE
echo "If the app fails to start, install runtime deps manually:" >> $LOG_FILE
echo "  Ubuntu/Debian: sudo apt install libxcb-cursor0 libxcb-xinerama0 libxkbcommon-x11-0" >> $LOG_FILE
echo "  Fedora: sudo dnf install libxcb libxcb-xinerama libxkbcommon-x11" >> $LOG_FILE
echo "  Arch: sudo pacman -S libxcb xcb-util-cursor xcb-util-wm xcb-util-image xcb-util-keysyms xcb-util-renderutil xcb-util-xrm" >> $LOG_FILE

sudo killall -9 "${APP_NAME}-service" 2>> $LOG_FILE
sudo killall -9 "FBLink-service" 2>> $LOG_FILE

# Clean old and current service registrations before reinstalling.
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

sudo chmod -R a-w $APP_PATH/

sudo cp $APP_PATH/$APP_NAME.service /etc/systemd/system/ >> $LOG_FILE
sudo systemctl daemon-reload >> $LOG_FILE 2>&1

# Install/enable service unit (fallback to legacy names if needed)
SERVICE_UNIT_SRC="$APP_PATH/$APP_NAME.service"
if [ ! -f "$SERVICE_UNIT_SRC" ] && [ -f "$APP_PATH/AmneziaVPN.service" ]; then
        SERVICE_UNIT_SRC="$APP_PATH/AmneziaVPN.service"
fi
if [ -f "$SERVICE_UNIT_SRC" ]; then
        sudo cp "$SERVICE_UNIT_SRC" /etc/systemd/system/ >> $LOG_FILE
        sudo systemctl daemon-reload >> $LOG_FILE 2>&1
        if [ -f "$APP_PATH/service/${APP_NAME}-service.sh" ]; then
                sudo chmod 755 "$APP_PATH/service/${APP_NAME}-service.sh" >> $LOG_FILE 2>&1
        elif [ -f "$APP_PATH/service/FBLinkVPN-service.sh" ]; then
                sudo chmod 755 "$APP_PATH/service/FBLinkVPN-service.sh" >> $LOG_FILE 2>&1
        fi
        sudo systemctl start $APP_NAME >> $LOG_FILE
        sudo systemctl enable $APP_NAME >> $LOG_FILE
else
        echo "WARN: service unit file not found at $APP_PATH" >> $LOG_FILE
fi

# Create app launchers
APP_LAUNCHER="$APP_PATH/client/$APP_NAME.sh"
ALT_LAUNCHER="$APP_PATH/client/$APP_BIN_NAME.sh"
if [ -f "$ALT_LAUNCHER" ] && [ ! -f "$APP_LAUNCHER" ]; then
        APP_LAUNCHER="$ALT_LAUNCHER"
fi
if [ -f "$APP_LAUNCHER" ]; then
        sudo chmod 555 "$APP_LAUNCHER" >> $LOG_FILE
        sudo ln -sfn "$APP_LAUNCHER" /usr/local/sbin/$APP_NAME >> $LOG_FILE
        sudo ln -sfn "$APP_LAUNCHER" /usr/local/bin/$APP_NAME >> $LOG_FILE
        sudo ln -sfn "$APP_LAUNCHER" /usr/local/bin/$APP_BIN_NAME >> $LOG_FILE
else
        echo "WARN: launcher not found in $APP_PATH/client" >> $LOG_FILE
fi

echo "user desktop creation loop started" >> $LOG_FILE
# Remove stale desktop entries from legacy Amnezia builds so shell caches pick the new icon.
sudo rm -f "$APP_PATH/AmneziaVPN.desktop" >> $LOG_FILE 2>&1
sudo rm -f /usr/share/applications/AmneziaVPN.desktop >> $LOG_FILE 2>&1
sudo rm -f /usr/local/share/applications/AmneziaVPN.desktop >> $LOG_FILE 2>&1
sudo rm -f /usr/share/applications/amneziavpn.desktop >> $LOG_FILE 2>&1
if [ -f "$APP_PATH/$APP_NAME.desktop" ]; then
        sudo cp "$APP_PATH/$APP_NAME.desktop" /usr/share/applications/ >> $LOG_FILE
elif [ -f "$APP_PATH/FBLinkVPN.desktop" ]; then
        sudo cp "$APP_PATH/FBLinkVPN.desktop" /usr/share/applications/ >> $LOG_FILE
elif [ -f "$APP_PATH/AmneziaVPN.desktop" ]; then
        sudo cp "$APP_PATH/AmneziaVPN.desktop" /usr/share/applications/$APP_NAME.desktop >> $LOG_FILE
        sudo sed -i 's/^Name=.*/Name=FBLink VPN/' /usr/share/applications/$APP_NAME.desktop >> $LOG_FILE 2>&1
        sudo sed -i 's|^Exec=.*|Exec=FBLinkVPN|' /usr/share/applications/$APP_NAME.desktop >> $LOG_FILE 2>&1
        sudo sed -i 's|^Icon=.*|Icon=/usr/share/pixmaps/FBLink.png|' /usr/share/applications/$APP_NAME.desktop >> $LOG_FILE 2>&1
fi
sudo cp $APP_PATH/FBLink.png /usr/share/pixmaps/ >> $LOG_FILE
sudo chmod 555 /usr/share/applications/$APP_NAME.desktop >> $LOG_FILE

echo "user desktop creation loop ended" >> $LOG_FILE

# Clean legacy icons and refresh caches
if [ -f /usr/share/pixmaps/AmneziaVPN.png ]; then
        sudo rm -f /usr/share/pixmaps/AmneziaVPN.png >> $LOG_FILE 2>&1
fi
if command -v update-desktop-database >/dev/null 2>&1; then
        sudo update-desktop-database /usr/share/applications >> $LOG_FILE 2>&1
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        sudo gtk-update-icon-cache -f /usr/share/icons/hicolor >> $LOG_FILE 2>&1 || true
fi

# Create a desktop launcher for the current user as well.
TARGET_USER="$SUDO_USER"
if [ -z "$TARGET_USER" ] || [ "$TARGET_USER" = "root" ]; then
        TARGET_USER=$(logname 2>/dev/null || true)
fi
if [ -n "$TARGET_USER" ] && [ "$TARGET_USER" != "root" ]; then
        TARGET_HOME=$(getent passwd "$TARGET_USER" | cut -d: -f6)
        TARGET_DESKTOP="$TARGET_HOME/Desktop"
        sudo rm -f "$TARGET_HOME/.local/share/applications/AmneziaVPN.desktop" >> $LOG_FILE 2>&1
        sudo rm -f "$TARGET_HOME/Desktop/AmneziaVPN.desktop" >> $LOG_FILE 2>&1
        sudo rm -f "$TARGET_HOME/Desktop/Amnezia VPN.desktop" >> $LOG_FILE 2>&1
        if [ -f "/usr/share/applications/$APP_NAME.desktop" ] && [ -d "$TARGET_DESKTOP" ]; then
                USER_DESKTOP_FILE="$TARGET_DESKTOP/FBLink VPN.desktop"
                sudo cp "/usr/share/applications/$APP_NAME.desktop" "$USER_DESKTOP_FILE" >> $LOG_FILE 2>&1
                sudo chown "$TARGET_USER:$TARGET_USER" "$USER_DESKTOP_FILE" >> $LOG_FILE 2>&1
                sudo chmod 755 "$USER_DESKTOP_FILE" >> $LOG_FILE 2>&1
                echo "desktop launcher created for $TARGET_USER at $USER_DESKTOP_FILE" >> $LOG_FILE
        fi
fi

if command -v steamos-readonly &> /dev/null; then
        sudo steamos-readonly enable >> $LOG_FILE
        echo "steamos-readonly enabled" >> $LOG_FILE
fi

date >> $LOG_FILE
echo "Service status:" >> $LOG_FILE
sudo systemctl status $APP_NAME >> $LOG_FILE
echo "Please reboot your computer to apply VPN networking changes." >> $LOG_FILE
echo "Please reboot your computer to apply VPN networking changes."
date >> $LOG_FILE
echo "Script finished" >> $LOG_FILE
exit 0
