# Amnezia VPN
## _The best client for self-hosted VPN_

[![Build Status](https://travis-ci.com/amnezia-vpn/desktop-client.svg?branch=master)](https://travis-ci.com/amnezia-vpn/desktop-client)

Amnezia is a VPN client with the key feature of deploying your own VPN server on you virtual server.

## Features
- Very easy to use - enter your ip address, ssh login and password, and Amnezia client will automatically install VPN docker containers to your server and connect to VPN.
- OpenVPN and OpenVPN over ShadowSocks protocols support. 
- Custom VPN routing mode support - add any sites to client to enable VPN only for them.
- Windows and MacOS support.
- Unsecure sharing connection profile for family use.

## Tech

AmneziaVPN uses a number of open source projects to work:

- [OpenSSL](https://www.openssl.org/)
- [OpenVPN](https://openvpn.net/)
- [ShadowSocks](https://shadowsocks.org/)
- [Qt](https://www.qt.io/)
- [EasyRSA](https://github.com/OpenVPN/easy-rsa) - part of OpenVPN
- [CygWin](https://www.cygwin.com/) - only for Windiws, used for launching EasyRSA scripts
- [QtSsh](https://github.com/jaredtao/QtSsh) - forked form Qt Creator
- and more...

## Development

Want to contribute? Welcome!
Use Qt Creator for fast developing.

### Building sources and deployment
Easiest way to build your own executables - is to fork project and configure [Travis CI](https://travis-ci.com/)
Or you can build sources manually using Qt Creator. Qt >= 14.2 supported.
Look to the `build_macos.sh` and `build_windows.bat` scripts in `deploy` folder for details.

## License
GPL v.3

## Contacts
[https://t.me/amnezia_vpn_en](https://t.me/amnezia_vpn_en) - Telegram support channel (English)
[https://t.me/amnezia_vpn](https://t.me/amnezia_vpn) - Telegram support channel (Russian)
[https://signal.group/...](https://signal.group/#CjQKIB2gUf8QH_IXnOJMGQWMDjYz9cNfmRQipGWLFiIgc4MwEhAKBONrSiWHvoUFbbD0xwdh) - Signal channel
[https://amnezia.org](https://amnezia.org) - project website

## Donate
Bitcoin: bc1qn9rhsffuxwnhcuuu4qzrwp4upkrq94xnh8r26u
Buy coffee for AmneziaVPN : [https://ko-fi.com/amnezia_vpn](https://ko-fi.com/amnezia_vpn)
