#!/bin/bash

readarray -t lines < $1
current_login=${lines[0]}
current_password=${lines[1]}

credentials_file_path=/opt/amnezia/openvpn/auth_credentials.txt

saved_login=$(awk 'NR==1' $credentials_file_path)
saved_password=$(awk 'NR==2' $credentials_file_path)

if [ "$current_login" == "$saved_login" ] && [ "$current_password" == "$saved_password" ]; then
  exit 0
fi
exit 1
