#!/bin/bash

# Mac name-resolution updater based on @cl's script here:
# https://blog.netnerds.net/2011/10/openvpn-update-client-dns-on-mac-os-x-using-from-the-command-line/
# Openvpn envvar parsing taken from the script in debian's openvpn package.
# Smushed together and improved by @andrewgdotcom.

# Parses DHCP options from openvpn to update resolv.conf
# To use set as 'up' and 'down' script in your openvpn *.conf:
# up /etc/openvpn/update-resolv-conf
# down /etc/openvpn/update-resolv-conf

[ "$script_type" ] || exit 0
[ "$dev" ] || exit 0

PATH=$PATH:/usr/sbin/
NMSRVRS=()
SRCHS=()

# Get adapter list
IFS=$'\n' read -d '' -ra adapters < <(networksetup -listallnetworkservices |grep -v denotes) || true


split_into_parts()
{
        part1="$1"
        part2="$2"
        part3="$3"
}

update_all_dns()
{
        for adapter in "${adapters[@]}"
        do
        echo updating dns for $adapter
        # set dns server to the vpn dns server
        if [[ "${SRCHS[@]}" ]]; then
			networksetup -setsearchdomains "$adapter" "${SRCHS[@]}"
        fi
        if [[ "${NMSRVRS[@]}" ]]; then
			networksetup -setdnsservers "$adapter" "${NMSRVRS[@]}"
		fi
        done
}


disable_ipv6() {

    if [ "$OSVER" = "10.4" ] ; then
        exit
    fi

    # Get list of services and remove the first line which contains a heading
    dipv6_services="$( networksetup  -listallnetworkservices | sed -e '1,1d')"

    # Go through the list disabling IPv6 for enabled services, and outputting lines with the names of the services
    printf %s "$dipv6_services
" | \
    while IFS= read -r dipv6_service ; do

        # If first character of a line is an asterisk, the service is disabled, so we skip it
        if [ "${dipv6_service:0:1}" != "*" ] ; then
            dipv6_ipv6_status="$( networksetup -getinfo "$dipv6_service" | grep 'IPv6: ' | sed -e 's/IPv6: //')"
            if [ "$dipv6_ipv6_status" = "Automatic" ] ; then
                networksetup -setv6off "$dipv6_service"
                echo "$dipv6_service"
            fi
        fi

    done
}

enable_ipv6() {

    if [ "$OSVER" = "10.4" ] ; then
        exit
    fi

    # Get list of services and remove the first line which contains a heading
    dipv6_services="$( networksetup  -listallnetworkservices | sed -e '1,1d')"

    # Go through the list disabling IPv6 for enabled services, and outputting lines with the names of the services
    printf %s "$dipv6_services
" | \
    while IFS= read -r dipv6_service ; do

        # If first character of a line is an asterisk, the service is disabled, so we skip it
        if [ "${dipv6_service:0:1}" != "*" ] ; then
            networksetup -setv6automatic "$dipv6_service"
            echo "$dipv6_service"
        fi

    done
}


clear_all_dns()
{
        for adapter in "${adapters[@]}"
        do
        echo updating dns for $adapter
        networksetup -setdnsservers "$adapter" empty
        networksetup -setsearchdomains "$adapter" empty
        done
}

case "$script_type" in
  up)
        for optionvarname in ${!foreign_option_*} ; do
                option="${!optionvarname}"
                echo "$option"
                split_into_parts $option
                if [ "$part1" = "dhcp-option" ] ; then
                        if [ "$part2" = "DNS" ] ; then
                                NMSRVRS=(${NMSRVRS[@]} $part3)
                        elif [ "$part2" = "DOMAIN" ] ; then
                                SRCHS=(${SRCHS[@]} $part3)
                        fi
                fi
        done
        update_all_dns
        disable_ipv6
        ;;
  down)
        clear_all_dns
        enable_ipv6 
        ;;
esac
