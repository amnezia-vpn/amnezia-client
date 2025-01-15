# Amnezia VPN

### _Лучший клиент для создания VPN на собственном сервере_

[![Build Status](https://github.com/amnezia-vpn/amnezia-client/actions/workflows/deploy.yml/badge.svg?branch=dev)](https://github.com/amnezia-vpn/amnezia-client/actions/workflows/deploy.yml?query=branch:dev)
[![Gitpod ready-to-code](https://img.shields.io/badge/Gitpod-ready--to--code-blue?logo=gitpod)](https://gitpod.io/#https://github.com/amnezia-vpn/amnezia-client)

### [English](https://github.com/amnezia-vpn/amnezia-client/blob/dev/README.md) | Русский
[AmneziaVPN](https://amnezia.org) — это open sourse VPN-клиент, ключевая особенность которого заключается в возможности развернуть собственный VPN на вашем сервере.

[![Image](https://github.com/amnezia-vpn/amnezia-client/blob/dev/metadata/img-readme/uipic4.png)](https://amnezia.org)

### [Сайт](https://amnezia.org) | [Зеркало на сайт](https://storage.googleapis.com/kldscp/amnezia.org) | [Документация](https://docs.amnezia.org) | [Решение проблем](https://docs.amnezia.org/troubleshooting)

> [!TIP]
> Если [сайт Amnezia](https://amnezia.org) заблокирован в вашем регионе, вы можете воспользоваться [ссылкой на зеркало](https://storage.googleapis.com/kldscp/amnezia.org).

<a href="https://storage.googleapis.com/kldscp/amnezia.org/downloads"><img src="https://github.com/amnezia-vpn/amnezia-client/blob/dev/metadata/img-readme/download-website-ru.svg" width="150" style="max-width: 100%; margin-right: 10px"></a>


[Все релизы](https://github.com/amnezia-vpn/amnezia-client/releases)

<br/>

<a href="https://www.testiny.io"><img src="https://github.com/amnezia-vpn/amnezia-client/blob/dev/metadata/img-readme/testiny.png" height="28px"></a>

## Особенности

- Простой в использовании — введите IP-адрес, SSH-логин и пароль, и Amnezia автоматически установит VPN-контейнеры Docker на ваш сервер и подключится к VPN.
- Классические VPN-протоколы: OpenVPN, WireGuard и IKEv2.
- Протоколы с маскировкой трафика (обфускацией): OpenVPN с плагином [Cloak](https://github.com/cbeuw/Cloak), Shadowsocks (OpenVPN over Shadowsocks), [AmneziaWG](https://docs.amnezia.org/documentation/amnezia-wg/) and XRay.
- Поддержка Split Tunneling — добавляйте любые сайты или приложения в список, чтобы включить VPN только для них.
- Поддерживает платформы: Windows, MacOS, Linux, Android, iOS.
- Поддержка конфигурации протокола AmneziaWG на [бета-прошивке Keenetic](https://docs.keenetic.com/ua/air/kn-1611/en/6319-latest-development-release.html#UUID-186c4108-5afd-c10b-f38a-cdff6c17fab3_section-idm33192196168192-improved).

## Ссылки

- [https://amnezia.org](https://amnezia.org) - Веб-сайт проекта | [Альтернативная ссылка (зеркало)](https://storage.googleapis.com/kldscp/amnezia.org)
- [https://docs.amnezia.org](https://docs.amnezia.org) - Документация
- [https://www.reddit.com/r/AmneziaVPN](https://www.reddit.com/r/AmneziaVPN) - Reddit  
- [https://t.me/amnezia_vpn_en](https://t.me/amnezia_vpn_en) - Канал поддржки в Telegram (Английский)
- [https://t.me/amnezia_vpn_ir](https://t.me/amnezia_vpn_ir) - Канал поддржки в Telegram (Фарси)
- [https://t.me/amnezia_vpn_mm](https://t.me/amnezia_vpn_mm) - Канал поддржки в Telegram (Мьянма) 
- [https://t.me/amnezia_vpn](https://t.me/amnezia_vpn) - Канал поддржки в Telegram  (Русский)
- [https://vpnpay.io/en/amnezia-premium/](https://vpnpay.io/en/amnezia-premium/) - Amnezia Premium | [Зеркало](https://storage.googleapis.com/kldscp/vpnpay.io/ru/amnezia-premium\)

## Технологии

AmneziaVPN использует несколько проектов с открытым исходным кодом:

- [OpenSSL](https://www.openssl.org/)
- [OpenVPN](https://openvpn.net/)
- [Shadowsocks](https://shadowsocks.org/)
- [Qt](https://www.qt.io/)
- [LibSsh](https://libssh.org)
- и другие...

## Проверка исходного кода
После клонирования репозитория обязательно загрузите все подмодули.

```bash
git submodule update --init --recursive
```


## Разработка
Хотите внести свой вклад? Добро пожаловать!

### Помощь с переводами

Загрузите самые актуальные файлы перевода.

Перейдите на [вкладку "Actions"](https://github.com/amnezia-vpn/amnezia-client/actions?query=is%3Asuccess+branch%3Adev), нажмите на первую строку. Затем прокрутите вниз до раздела "Artifacts" и скачайте "AmneziaVPN_translations".

Распакуйте этот файл. Каждый файл с расширением *.ts содержит строки для соответствующего языка.

Переведите или исправьте строки в одном или нескольких файлах *.ts и загрузите их обратно в этот репозиторий в папку ``client/translations``. Это можно сделать через веб-интерфейс или любым другим знакомым вам способом.

### Сборка исходного кода и деплой
Проверьте папку deploy для скриптов сборки.

### Как собрать iOS-приложение из исходного кода на MacOS
1. Убедитесь, что у вас установлен XCode версии 14 или выше.
2. Для генерации проекта XCode используется QT. Требуется версия QT 6.6.2. Установите QT для MacOS здесь или через QT Online Installer. Необходимые модули:
- MacOS
- iOS
- Модуль совместимости с Qt 5
- Qt Shader Tools
- Дополнительные библиотеки:
 - Qt Image Formats
 - Qt Multimedia
 - Qt Remote Objects

   
3. Установите CMake, если это необходимо. Рекомендуемая версия — 3.25. Скачать CMake можно здесь.
4. Установите Go версии >= v1.16. Если Go ещё не установлен, скачайте его с  [официального сайта](https://golang.org/dl/) или используйте Homebrew. Установите gomobile:

```bash
export PATH=$PATH:~/go/bin
go install golang.org/x/mobile/cmd/gomobile@latest
gomobile init
```

5. Соберите проект:
```bash
export QT_BIN_DIR="<PATH-TO-QT-FOLDER>/Qt/<QT-VERSION>/ios/bin"
export QT_MACOS_ROOT_DIR="<PATH-TO-QT-FOLDER>/Qt/<QT-VERSION>/macos"
export QT_IOS_BIN=$QT_BIN_DIR
export PATH=$PATH:~/go/bin
mkdir build-ios
$QT_IOS_BIN/qt-cmake . -B build-ios -GXcode -DQT_HOST_PATH=$QT_MACOS_ROOT_DIR
```
Замените <PATH-TO-QT-FOLDER> и <QT-VERSION> на ваши значения.

Если появляется ошибка gomobile: command not found, убедитесь, что PATH настроен на папку bin, где установлен gomobile:
```bash
export PATH=$(PATH):/path/to/GOPATH/bin
```

6. Откройте проект в XCode. Теперь вы можете тестировать, архивировать или публиковать приложение.
   
Если сборка завершится с ошибкой:
```
make: *** 
[$(PROJECTDIR)/client/build/AmneziaVPN.build/Debug-iphoneos/wireguard-go-bridge/goroot/.prepared] 
Error 1
```
Добавьте пользовательскую переменную PATH в настройки сборки для целей AmneziaVPN и WireGuardNetworkExtension с ключом `PATH` и значением `${PATH}/path/to/bin/folder/with/go/executable`, e.g. `${PATH}:/usr/local/go/bin`.

Если ошибка повторяется на Mac с M1, установите версию CMake для архитектуры ARM:
```
arch -arm64 brew install cmake
```

 При первой попытке сборка может завершиться с ошибкой source files not found. Это происходит из-за параллельной компиляции зависимостей в XCode. Просто перезапустите сборку.


## Как собрать Android-приложение
Сборка тестировалась на MacOS. Требования:
- JDK 11
- Android SDK 33
- CMake 3.25.0
  
Установите QT, QT Creator и Android Studio.
Настройте QT Creator:

- В меню QT Creator перейдите в  `QT Creator` -> `Preferences` -> `Devices` ->`Android`.
- Укажите путь к JDK 11.
- Укажите путь к Android SDK (`$ANDROID_HOME`)

Если вы сталкиваетесь с ошибками, связанными с отсутствием SDK или сообщением «SDK manager not running», их нельзя исправить просто корректировкой путей. Если у вас есть несколько свободных гигабайт на диске, вы можете позволить Qt Creator установить все необходимые компоненты, выбрав пустую папку для расположения Android SDK и нажав кнопку **Set Up SDK**. Учтите: это установит второй Android SDK и NDK на вашем компьютере!

Убедитесь, что настроена правильная версия CMake: перейдите в **Qt Creator -> Preferences** и в боковом меню выберите пункт **Kits**. В центральной части окна, на вкладке **Kits**, найдите запись для инструмента **CMake Tool**. Если выбранная по умолчанию версия CMake ниже 3.25.0, установите на свою систему CMake версии 3.25.0 или выше, а затем выберите опцию **System CMake at <путь>** из выпадающего списка. Если этот пункт отсутствует, это может означать, что вы еще не установили CMake, или Qt Creator не смог найти путь к нему. В таком случае в окне **Preferences** перейдите в боковое меню **CMake**, затем во вкладку **Tools** в центральной части окна и нажмите кнопку **Add**, чтобы указать путь к установленному CMake.

Убедитесь, что для вашего проекта выбрана Android Platform SDK 33: в главном окне на боковой панели выберите пункт **Projects**, и слева вы увидите раздел **Build & Run**, показывающий различные целевые Android-платформы. Вы можете выбрать любую из них, так как настройка проекта Amnezia VPN разработана таким образом, чтобы все Android-цели могли быть собраны. Перейдите в подраздел **Build** и прокрутите центральную часть окна до раздела **Build Steps**. Нажмите **Details** в заголовке **Build Android APK** (кнопка **Details** может быть скрыта, если окно Qt Creator не запущено в полноэкранном режиме!). Вот здесь выберите **android-33** в качестве Android Build Platform SDK.

### Разработка Android-компонентов

После сборки QT Creator копирует проект в отдельную папку, например, `build-amnezia-client-Android_Qt_<version>_Clang_<architecture>-<BuildType>`. Для разработки Android-компонентов откройте сгенерированный проект в Android Studio, указав папку `build-amnezia-client-Android_Qt_<version>_Clang_<architecture>-<BuildType>/client/android-build` в качестве корневой.
Изменения в сгенерированном проекте нужно вручную перенести в репозиторий. После этого можно коммитить изменения.
Если возникают проблемы со сборкой в QT Creator после работы в Android Studio, выполните команду `./gradlew clean` в корневой папке сгенерированного проекта (`<path>/client/android-build/.`).


## Лицензия

GPL v3.0

## Донаты

Patreon: [https://www.patreon.com/amneziavpn](https://www.patreon.com/amneziavpn)

Bitcoin: bc1qmhtgcf9637rl3kqyy22r2a8wa8laka4t9rx2mf <br>
USDT BEP20: 0x6abD576765a826f87D1D95183438f9408C901bE4 <br>
USDT TRC20: TELAitazF1MZGmiNjTcnxDjEiH5oe7LC9d <br>
XMR: 48spms39jt1L2L5vyw2RQW6CXD6odUd4jFu19GZcDyKKQV9U88wsJVjSbL4CfRys37jVMdoaWVPSvezCQPhHXUW5UKLqUp3 <br> 
TON: UQDpU1CyKRmg7L8mNScKk9FRc2SlESuI7N-Hby4nX-CcVmns

## Благодарности

Этот проект тестируется с помощью BrowserStack.
Мы выражаем благодарность [BrowserStack](https://www.browserstack.com) за поддержку нашего проекта.
