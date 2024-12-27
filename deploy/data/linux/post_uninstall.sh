#!/bin/bash

APP_NAME=AmneziaVPN
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

ls /opt/AmneziaVPN/client/lib/* | while IFS=: read -r dir; do
	sudo unlink $dir  >> $LOG_FILE
done

if sudo systemctl is-active --quiet $APP_NAME; then
	sudo systemctl stop $APP_NAME >> $LOG_FILE
fi

if sudo systemctl is-enabled --quiet $APP_NAME; then
	sudo systemctl disable $APP_NAME >> $LOG_FILE
fi

if test -f /etc/systemd/system/$APP_NAME.service; then
	sudo rm -rf /etc/systemd/system/$APP_NAME.service >> $LOG_FILE
fi

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

if test -f /usr/local/sbin/$APP_NAME; then
        sudo rm -f /usr/local/sbin/$APP_NAME >> $LOG_FILE
fi

if test -f /usr/share/applications/$APP_NAME.desktop; then
	sudo rm -f /usr/share/applications/$APP_NAME.desktop >> $LOG_FILE

fi

if test -f /usr/share/pixmaps/$APP_NAME.png; then
	sudo rm -f /usr/share/pixmaps/$APP_NAME.png >> $LOG_FILE

fi

if command -v steamos-readonly &> /dev/null; then
	sudo steamos-readonly enable >> $LOG_FILE
	echo "steamos-readonly enabled" >> $LOG_FILE
fi

date >> $LOG_FILE
echo "Service after uninstall status:" >> $LOG_FILE
sudo systemctl status $APP_NAME >> $LOG_FILE
date >> $LOG_FILE
echo "Script finished" >> $LOG_FILE
