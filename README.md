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
- [QtSsh](https://github.com/jaredtao/QtSsh) - forked form Qt Creator
- and more...

## Checking out the source code

Make sure to pull all submodules after checking out the repo.

```bash
git submodule update --init
```

## Development

Want to contribute? Welcome!

### Building sources and deployment
Easiest way to build your own executables - is to fork project and configure [Travis CI](https://travis-ci.com/)  
Or you can build sources manually using Qt Creator. Qt >= 14.2 supported.  
Look to the `build_macos.sh` and `build_windows.bat` scripts in `deploy` folder for details.

### How to build iOS app from source code on MacOS

1. First, make sure you have [XCode](https://developer.apple.com/xcode/) installed, 
at least version 12 or higher.

2. We use `qmake` to generate the XCode project and then we "patch" it to add
extra components such as the wireguard, the browser bridge and so on. We patch
the XCode project using [xcodeproj](https://github.com/CocoaPods/Xcodeproj). To
install it:
```bash
gem install xcodeproj # probably you want to run this command with `sudo`
```
3. You also need to install go >= v1.16. If you don't have it done already,
download go from the [official website](https://golang.org/dl/) or use Homebrew. 
Latest version is recommended.

4. Navigate inside client folder and generate the XCode project using our script:
```bash
cd client
./scripts/apple_compile.sh ios
```

If you have more than one version of Qt installed, you'll most likely get
a "`qmake` cannot be found in your `$PATH`" error. In this case run this script 
using QT\IOS\_BIN env to set the path for the Qt5 macos build bin folder.
For example, the path could look like this:
```bash
QT_IOS_BIN="/Users/username/Qt/6.4.1/ios/bin" ./scripts/apple_compile.sh ios
```

If you get `gomobile: command not found` make sure to set PATH to the location 
of the bin folder where gomobile was installed. Usually, it's in `GOPATH`.
```bash
export PATH=$(PATH):/path/to/GOPATH/bin
```

5. Xcode should automatically open. You can then run/test/archive/ship the app.

If build fails with the following error
```
make: *** 
[$(PROJECTDIR)/client/build/AmneziaVPN.build/Debug-iphoneos/wireguard-go-bridge/goroot/.prepared] 
Error 1
```
Add a user defined variable to both AmneziaVPN and WireGuardNetworkExtension targets' build settings with
key `PATH` and value `${PATH}/path/to/bin/folder/with/go/executable`, e.g. `${PATH}:/usr/local/go/bin`.

Build might fail with "source files not found" error the first time you try it, because modern XCode build system compiles
dependencies in parallel, and some dependencies end up being built after the ones that
require them. In this case simply restart the build.


## License
GPL v.3

## Contacts
[https://t.me/amnezia_vpn_en](https://t.me/amnezia_vpn_en) - Telegram support channel (English)  
[https://t.me/amnezia_vpn](https://t.me/amnezia_vpn) - Telegram support channel (Russian)  
[https://signal.group/...](https://signal.group/#CjQKIB2gUf8QH_IXnOJMGQWMDjYz9cNfmRQipGWLFiIgc4MwEhAKBONrSiWHvoUFbbD0xwdh) - Signal channel  
[https://amnezia.org](https://amnezia.org) - project website  

## Donate
Bitcoin: bc1qn9rhsffuxwnhcuuu4qzrwp4upkrq94xnh8r26u  
XMR: 48spms39jt1L2L5vyw2RQW6CXD6odUd4jFu19GZcDyKKQV9U88wsJVjSbL4CfRys37jVMdoaWVPSvezCQPhHXUW5UKLqUp3  
payeer.com: P2561305  
ko-fi.com: [https://ko-fi.com/amnezia_vpn](https://ko-fi.com/amnezia_vpn)  
