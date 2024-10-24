# Amnezia VPN
## _The best client for self-hosted VPN_

[![Build Status](https://github.com/amnezia-vpn/amnezia-client/actions/workflows/deploy.yml/badge.svg?branch=dev)](https://github.com/amnezia-vpn/amnezia-client/actions/workflows/deploy.yml?query=branch:dev)
[![Gitpod ready-to-code](https://img.shields.io/badge/Gitpod-ready--to--code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/amnezia-vpn/amnezia-client)

Amnezia is an open-source VPN client, with a key feature that enables you to deploy your own VPN server on your server.

![Image](https://github.com/amnezia-vpn/amnezia-client/blob/dev/metadata/img-readme/uipic4.png)

<br>

<a href="https://amnezia.org/downloads"><img src="https://github.com/amnezia-vpn/amnezia-client/blob/dev/metadata/img-readme/download.png" width="150" style="max-width: 100%;"></a>
<a href="https://play.google.com/store/search?q=amnezia+vpn&c=apps"><img src="https://github.com/amnezia-vpn/amnezia-client/blob/dev/metadata/img-readme/play.png" width="150" style="max-width: 100%;"></a>
<a href="https://apps.apple.com/us/app/amneziavpn/id1600529900"><img src="https://github.com/amnezia-vpn/amnezia-client/blob/dev/metadata/img-readme/apl.png" width="150" style="max-width: 100%;"></a>

[Alternative download link (mirror)](https://storage.googleapis.com/kldscp/amnezia.org/downloads)

[All releases](https://github.com/amnezia-vpn/amnezia-client/releases)

<br>

<a href="https://www.testiny.io"><img src="https://github.com/amnezia-vpn/amnezia-client/blob/dev/metadata/img-readme/testiny.png" height="28px"></a>

## Features

- Very easy to use - enter your IP address, SSH login, password and Amnezia will automatically install VPN docker containers to your server and connect to the VPN.
- Classic VPN-protocols: OpenVPN, WireGuard and IKEv2 protocols.
- Protocols with traffic Masking (Obfuscation): OpenVPN over [Cloak](https://github.com/cbeuw/Cloak) plugin, Shadowsocks (OpenVPN over Shadowsocks), [AmneziaWG](https://docs.amnezia.org/documentation/amnezia-wg/) and XRay.
- Split tunneling support - add any sites to the client to enable VPN only for them or add Apps (only for Android and Desktop).
- Windows, MacOS, Linux, Android, iOS releases.
- Support for AmneziaWG protocol configuration on [Keenetic beta firmware](https://docs.keenetic.com/ua/air/kn-1611/en/6319-latest-development-release.html#UUID-186c4108-5afd-c10b-f38a-cdff6c17fab3_section-idm33192196168192-improved).

## Links

- [https://amnezia.org](https://amnezia.org) - project website | [Alternative link (mirror)](https://storage.googleapis.com/kldscp/amnezia.org)
- [https://www.reddit.com/r/AmneziaVPN](https://www.reddit.com/r/AmneziaVPN) - Reddit  
- [https://t.me/amnezia_vpn_en](https://t.me/amnezia_vpn_en) - Telegram support channel (English) 
- [https://t.me/amnezia_vpn_ir](https://t.me/amnezia_vpn_ir) - Telegram support channel (Farsi) 
- [https://t.me/amnezia_vpn_mm](https://t.me/amnezia_vpn_mm) - Telegram support channel (Myanmar)  
- [https://t.me/amnezia_vpn](https://t.me/amnezia_vpn) - Telegram support channel (Russian)
- [https://vpnpay.io/en/amnezia-premium/](https://vpnpay.io/en/amnezia-premium/) - Amnezia Premium

## Tech

AmneziaVPN uses several open-source projects to work:

- [OpenSSL](https://www.openssl.org/)
- [OpenVPN](https://openvpn.net/)
- [Shadowsocks](https://shadowsocks.org/)
- [Qt](https://www.qt.io/)
- [LibSsh](https://libssh.org) - forked from Qt Creator
- and more...

## Checking out the source code

Make sure to pull all submodules after checking out the repo.

```bash
git submodule update --init --recursive
```

## Development

Want to contribute? Welcome!

### Help with translations

Download the most actual translation files.

Go to ["Actions" tab](https://github.com/amnezia-vpn/amnezia-client/actions?query=is%3Asuccess+branch%3Adev), click on the first line.
Then scroll down to the "Artifacts" section and download "AmneziaVPN_translations".

Unzip this file.
Each *.ts file contains strings for one corresponding language.

Translate or correct some strings in one or multiple *.ts files and commit them back to this repository into the ``client/translations`` folder.
You can do it via a web-interface or any other method you're familiar with.

### Building sources and deployment

Check deploy folder for build scripts. 

### How to build an iOS app from source code on MacOS

1. First, make sure you have [XCode](https://developer.apple.com/xcode/) installed, at least version 14 or higher.

2. We use QT to generate the XCode project. We need QT version 6.6.2. Install QT for MacOS [here](https://doc.qt.io/qt-6/macos.html) or [QT Online Installer](https://www.qt.io/download-open-source). Required modules:
   - MacOS
   - iOS
   - Qt 5 Compatibility Module
   - Qt Shader Tools
   - Additional Libraries:
     - Qt Image Formats
     - Qt Multimedia
     - Qt Remote Objects 

3. Install CMake if required. We recommend CMake version 3.25. You can install CMake [here](https://cmake.org/download/)

4. You also need to install go >= v1.16. If you don't have it installed already,
download go from the [official website](https://golang.org/dl/) or use Homebrew. 
The latest version is recommended. Install gomobile
```bash
export PATH=$PATH:~/go/bin
go install golang.org/x/mobile/cmd/gomobile@latest
gomobile init
```

5. Build the project
```bash
export QT_BIN_DIR="<PATH-TO-QT-FOLDER>/Qt/<QT-VERSION>/ios/bin"
export QT_MACOS_ROOT_DIR="<PATH-TO-QT-FOLDER>/Qt/<QT-VERSION>/macos"
export QT_IOS_BIN=$QT_BIN_DIR
export PATH=$PATH:~/go/bin
mkdir build-ios
$QT_IOS_BIN/qt-cmake . -B build-ios -GXcode -DQT_HOST_PATH=$QT_MACOS_ROOT_DIR
```
Replace PATH-TO-QT-FOLDER and QT-VERSION to your environment


If you get `gomobile: command not found` make sure to set PATH to the location 
of the bin folder where gomobile was installed. Usually, it's in `GOPATH`.
```bash
export PATH=$(PATH):/path/to/GOPATH/bin
```

6. Open the XCode project. You can then run /test/archive/ship the app.

If the build fails with the following error
```
make: *** 
[$(PROJECTDIR)/client/build/AmneziaVPN.build/Debug-iphoneos/wireguard-go-bridge/goroot/.prepared] 
Error 1
```
Add a user-defined variable to both AmneziaVPN and WireGuardNetworkExtension targets' build settings with
key `PATH` and value `${PATH}/path/to/bin/folder/with/go/executable`, e.g. `${PATH}:/usr/local/go/bin`.

if the above error persists on your M1 Mac, then most probably you need to install arch based CMake 
```
arch -arm64 brew install cmake
```

Build might fail with the "source files not found" error the first time you try it, because the modern XCode build system compiles dependencies in parallel, and some dependencies end up being built after the ones that
require them. In this case, simply restart the build.

## How to build the Android app

_Tested on Mac OS_

The Android app has the following requirements:
* JDK 11
* Android platform SDK 33
* CMake 3.25.0

After you have installed QT, QT Creator, and Android Studio, you need to configure QT Creator correctly.

- Click in the top menu bar on `QT Creator` -> `Preferences` -> `Devices` and select the tab `Android`.
- Set path to JDK 11
- Set path to Android SDK (`$ANDROID_HOME`)

In case you get errors regarding missing SDK or 'SDK manager not running', you cannot fix them by correcting the paths. If you have some spare GBs on your disk, you can let QT Creator install all requirements by choosing an empty folder for `Android SDK location` and clicking on `Set Up SDK`. Be aware: This will install a second Android SDK and NDK on your machine! 
Double-check that the right CMake version is configured:  Click on `QT Creator` -> `Preferences` and click on the side menu on `Kits`. Under the center content view's `Kits` tab, you'll find an entry for `CMake Tool`. If the default selected CMake version is lower than 3.25.0, install on your system CMake >= 3.25.0 and choose `System CMake at <path>` from the drop-down list. If this entry is missing, you either have not installed CMake yet or QT Creator hasn't found the path to it. In that case, click in the preferences window on the side menu item `CMake`, then on the tab `Tools` in the center content view, and finally on the button `Add` to set the path to your installed CMake. 
Please make sure that you have selected Android Platform SDK 33 for your project: click in the main view's side menu on `Projects`, and on the left, you'll see a section `Build & Run` showing different Android build targets. You can select any of them, Amnezia VPN's project setup is designed in a way that all Android targets will be built. Click on the targets submenu item `Build` and scroll in the center content view to `Build Steps`. Click on `Details` at the end of the headline `Build Android APK` (the `Details` button might be hidden in case the QT Creator Window is not running in full screen!). Here we are: Choose `android-33` as `Android Build Platform SDK`.

That's it! You should be ready to compile the project from QT Creator!

### Development flow

After you've hit the build button, QT-Creator copies the whole project to a folder in the repository parent directory. The folder should look something like `build-amnezia-client-Android_Qt_<version>_Clang_<architecture>-<BuildType>`.
If you want to develop Amnezia VPNs Android components written in Kotlin, such as components using system APIs, you need to import the generated project in Android Studio with `build-amnezia-client-Android_Qt_<version>_Clang_<architecture>-<BuildType>/client/android-build` as the projects root directory. While you should be able to compile the generated project from Android Studio, you cannot work directly in the repository's Android project. So whenever you are confident with your work in the generated project, you'll need to copy and paste the affected files to the corresponding path in the repository's Android project so that you can add and commit your changes!

You may face compiling issues in QT Creator after you've worked in Android Studio on the generated project. Just do a `./gradlew clean` in the generated project's root directory (`<path>/client/android-build/.`) and you should be good to go.

## License

GPL v3.0

## Donate

Patreon: [https://www.patreon.com/amneziavpn](https://www.patreon.com/amneziavpn)

Bitcoin: bc1q26eevjcg9j0wuyywd2e3uc9cs2w58lpkpjxq6p <br>
USDT BEP20: 0x6abD576765a826f87D1D95183438f9408C901bE4 <br>
USDT TRC20: TELAitazF1MZGmiNjTcnxDjEiH5oe7LC9d <br>
XMR: 48spms39jt1L2L5vyw2RQW6CXD6odUd4jFu19GZcDyKKQV9U88wsJVjSbL4CfRys37jVMdoaWVPSvezCQPhHXUW5UKLqUp3  

## Acknowledgments

This project is tested with BrowserStack.
We express our gratitude to [BrowserStack](https://www.browserstack.com) for supporting our project.
