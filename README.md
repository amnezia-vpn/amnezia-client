# Amnezia VPN
## _The best client for self-hosted VPN_

[![Build Status](https://github.com/amnezia-vpn/desktop-client/actions/workflows/deploy.yml/badge.svg?branch=dev)]

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

if above error still persists on you M1 Mac, then most probably you need to install arch based cmake 
```
arch -arm64 brew install cmake
```

Build might fail with "source files not found" error the first time you try it, because modern XCode build system compiles
dependencies in parallel, and some dependencies end up being built after the ones that
require them. In this case simply restart the build.

## How to build the Android app
_tested on Mac OS_

The Android app has the following requirements:
* JDK 11
* Android platform SDK 33
* cmake 3.25.0

After you have installed QT, QT Creator and Android Studio installed, you need to configure QT Creator correctly. Click in the top menu bar on `QT Creator` -> `Preferences` -> `Devices` and select the tab `Android`. 
    * set path to jdk 11
    * set path to Android SDK ($ANDROID_HOME)

In case you get errors regarding missing SDK or 'sdkmanager not running', you cannot fix them by correcting the paths and you have some spare GBs on your disk, you can let QT Creator install all requirements by choosing an empty folder for `Android SDK location` and click on `Set Up SDK`. Be aware: This will install a second Android SDK and NDK on your machine! 

Double check that the right cmake version is configured:  Click on `QT Creator` -> `Preferences` and click on the side menu on `Kits`. Under the center content view's `Kits` tab you'll find an entry `CMake Tool`. If the default selected CMake version is lower than 3.25.0, install on your system CMake >= 3.25.0 and choose `System CMake at <path>` from the drop down list. If this entry is missing, you either have not installed CMake yet or QT Creator hasn't found the path to it. In that case click in the preferences window on the side menu item `CMake`, then on the tab `Tools`in the center content view and finally on the Button `Add` to set the path to your installed CMake. 

Please make sure that you have selected Android Platform SDK 33 for your project: click in the main view's side menu on on `Projects`, on the left you'll see a section `Build & Run` showing different Android build targets. You can select any of them, Amnezia VPN's project setup is designed in a way that always all Android targets will be build. Click on the targets submenu item `Build` and scroll in the center content view to `Build Steps`. Click on `Details` at the end of the headline `Build Android APK` (The `Details` button might be hidden in case QT Creator Window is not running in full screen!). Here we are: choose `android-33` as `Android Build platform SDK`.

That's it you should be ready to compile the project from QT Creator!

### Development flow
After you've hit the build button, QT-Creator copies the whole project to a folder in the repositories parent directory. The folder should look something like `build-amnezia-client-Android_Qt_<version>_Clang_<architecture>-<BuildType>`.
If you want to develop Amnezia VPNs Android components written in Kotlin, such as components using system APIs, you need to import the generated project in Android Studio with `build-amnezia-client-Android_Qt_<version>_Clang_<architecture>-<BuildType>/client/android-build` as the projects root directory. While you should be able to compile the generated project from Android Studio, you cannot work directly in the repository's Android project. So whenever you are confident with your work in the generated project, you'll need to copy and paste the affected files to the corresponding path in the repositories Android project so that you can add and commit your changes!

You may face compiling issues in QT Creator after you've worked in Android Studio on the generated project. Just do a `./gradlew clean` in the generated project's root directory (`<path>/client/android-build/.`) and you should be good to continue.

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
