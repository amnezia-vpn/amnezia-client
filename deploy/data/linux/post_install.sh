#!/bin/bash

APP_NAME=AmneziaVPN
LOG_FOLDER=/var/log/$APP_NAME
LOG_FILE="$LOG_FOLDER/post-install.log"
APP_PATH=/opt/$APP_NAME

if ! test -f $LOG_FOLDER; then
        sudo mkdir $LOG_FOLDER
        echo "AmneziaVPN log dir created at /var/log/"
fi

if ! test -f $LOG_FILE; then
        touch $LOG_FILE
        echo "AmneziaVPN log file created at /var/log/AmneziaVPN/post-install.log"
fi

date > $LOG_FILE
echo "Script started" >> $LOG_FILE
sudo killall -9 $APP_NAME 2>> $LOG_FILE

if command -v steamos-readonly &> /dev/null; then
        sudo steamos-readonly disable >> $LOG_FILE
        echo "steamos-readonly disabled" >> $LOG_FILE
fi

if sudo systemctl is-active --quiet $APP_NAME; then
	sudo systemctl stop $APP_NAME >> $LOG_FILE
	sudo systemctl disable $APP_NAME >> $LOG_FILE
	sudo rm -rf /etc/systemd/system/$APP_NAME.service >> $LOG_FILE
fi

# Absolute Exec= in .desktop: Firefox/portal invoke handlers with a minimal PATH.
DESKTOP_IN_APP="$APP_PATH/$APP_NAME.desktop"
if [ -f "$DESKTOP_IN_APP" ]; then
	sudo sed -i "s|^Exec=.*|Exec=$APP_PATH/bin/$APP_NAME %u|" "$DESKTOP_IN_APP" >> $LOG_FILE 2>&1 || true
	sudo sed -i '1{/^#!\/usr\/bin\/env xdg-open$/d;}' "$DESKTOP_IN_APP" >> $LOG_FILE 2>&1 || true
fi

sudo chmod -R a-w $APP_PATH/

sudo cp $APP_PATH/$APP_NAME.service /etc/systemd/system/ >> $LOG_FILE

sudo systemctl start $APP_NAME >> $LOG_FILE
sudo systemctl enable $APP_NAME >> $LOG_FILE
sudo ln -sf $APP_PATH/bin/$APP_NAME /usr/local/sbin/$APP_NAME >> $LOG_FILE
sudo ln -sf $APP_PATH/bin/$APP_NAME /usr/local/bin/$APP_NAME >> $LOG_FILE

echo "user desktop creation loop started" >> $LOG_FILE
sudo cp $APP_PATH/$APP_NAME.desktop /usr/share/applications/ >> $LOG_FILE
sudo cp $APP_PATH/$APP_NAME.png /usr/share/pixmaps/ >> $LOG_FILE
sudo chmod 555 /usr/share/applications/$APP_NAME.desktop >> $LOG_FILE

if command -v update-desktop-database &> /dev/null; then
	sudo update-desktop-database /usr/share/applications >> $LOG_FILE 2>&1 || true
fi

register_vpn_scheme_for_user() {
	local user="$1"
	if [ -z "$user" ] || [ "$user" = "root" ]; then
		return
	fi
	if ! command -v xdg-mime &> /dev/null; then
		return
	fi
	local home
	home=$(getent passwd "$user" | cut -d: -f6)
	if [ -z "$home" ] || [ ! -d "$home" ]; then
		echo "skip xdg-mime for $user: no home" >> $LOG_FILE
		return
	fi
	echo "xdg-mime default for user $user" >> $LOG_FILE
	sudo -u "$user" env HOME="$home" \
		xdg-mime default "$APP_NAME.desktop" x-scheme-handler/vpn >> $LOG_FILE 2>&1 || true
}

if [ -n "$SUDO_USER" ] && [ "$SUDO_USER" != "root" ]; then
	register_vpn_scheme_for_user "$SUDO_USER"
elif [ -n "$USER" ] && [ "$USER" != "root" ]; then
	register_vpn_scheme_for_user "$USER"
fi

echo "user desktop creation loop ended" >> $LOG_FILE

if command -v steamos-readonly &> /dev/null; then
        sudo steamos-readonly enable >> $LOG_FILE
        echo "steamos-readonly enabled" >> $LOG_FILE
fi

date >> $LOG_FILE
echo "Service status:" >> $LOG_FILE
sudo systemctl status $APP_NAME >> $LOG_FILE
date >> $LOG_FILE
echo "Script finished" >> $LOG_FILE
exit 0
