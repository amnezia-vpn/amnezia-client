# FBLink Rebrand Implementation Plan

> **For agentic workers:** REQUIRED: Use superpowers:subagent-driven-development (if subagents available) or superpowers:executing-plans to implement this plan. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Переименовать все упоминания MrFrake/DrFrake → FBLink во всём проекте (C++ контроллер, QML страницы, installer, CI/CD, сборочные скрипты).

**Architecture:** Чистый ребрендинг без изменения логики. C++ класс `MrFrakeController` → `FBLinkController`, QML context property `"MrFrakeController"` → `"FBLinkController"`, все QML файлы и enum-значения страниц переименовываются аналогично. Installer-пакет меняет домен с `org.mrfrakevpn.package` на `org.fblinkvpn.package`.

**Tech Stack:** Qt6/QML, C++, CMake, Go (backend не затрагивается), Qt Installer Framework, GitHub Actions

> **Примечание о URLs/контактах:** В `PageSettingsAbout.qml` и `PageMrFrakeSubscription.qml` есть контактные URL и email (`t.me/mrfrake_vpn`, `support@mrfrake.vpn`, `mrfrake.ru/terms`, `mrfrake.ru/privacy`). В плане они заменяются на FBLink-плейсхолдеры — **перед деплоем нужно заменить на реальные значения**.

---

## Chunk 1: C++ контроллер и CMake

### Task 1: Переименовать файлы контроллера и обновить содержимое

**Files:**
- Rename: `client/ui/controllers/api/mrfrake_controller.h` → `client/ui/controllers/api/fblink_controller.h`
- Rename: `client/ui/controllers/api/mrfrake_controller.cpp` → `client/ui/controllers/api/fblink_controller.cpp`

- [ ] **Step 1: Переименовать файл заголовка**

```bash
mv client/ui/controllers/api/mrfrake_controller.h \
   client/ui/controllers/api/fblink_controller.h
```

- [ ] **Step 2: Обновить fblink_controller.h — все символы**

Заменить в `client/ui/controllers/api/fblink_controller.h` (replace_all):
- `DRFRAKECONTROLLER_H` → `FBLINKCONTROLLER_H`
- `class MrFrakeController` → `class FBLinkController`
- `clearExistingMrFrakeServers` → `clearExistingFBLinkServers`

- [ ] **Step 3: Переименовать файл реализации**

```bash
mv client/ui/controllers/api/mrfrake_controller.cpp \
   client/ui/controllers/api/fblink_controller.cpp
```

- [ ] **Step 4: Обновить fblink_controller.cpp — имена символов**

Заменить в `client/ui/controllers/api/fblink_controller.cpp` (replace_all):
- `#include "mrfrake_controller.h"` → `#include "fblink_controller.h"`
- `MrFrakeController::` → `FBLinkController::`
- `clearExistingMrFrakeServers` → `clearExistingFBLinkServers`
- `existingMrFrakeServerIndices` → `existingFBLinkServerIndices`

- [ ] **Step 5: Обновить fblink_controller.cpp — строки данных**

Заменить в `client/ui/controllers/api/fblink_controller.cpp`:
- QSettings ключ: оба аргумента `"MrFrakeVPN", "MrFrakeVPN"` → `"FBLinkVPN", "FBLinkVPN"` (строка конструктора `QSettings(...)`)
- Строки сопоставления серверов в `fetchConfig()`: `"Mr.Frake VPN"` → `"FBLink VPN"` (предикаты `desc.startsWith(...)` и `name.startsWith(...)`)

- [ ] **Step 6: Commit**

```bash
git add client/ui/controllers/api/fblink_controller.h \
        client/ui/controllers/api/fblink_controller.cpp
git rm client/ui/controllers/api/mrfrake_controller.h \
       client/ui/controllers/api/mrfrake_controller.cpp
git commit -m "refactor: rename MrFrakeController -> FBLinkController"
```

---

### Task 2: Обновить CMake build-файлы

**Files:**
- Modify: `client/cmake/sources.cmake`
- Modify: `CMakeLists.txt` (корневой)

- [ ] **Step 1: Обновить sources.cmake**

Заменить в `client/cmake/sources.cmake` (два конкретных вхождения):
- `${CLIENT_ROOT_DIR}/ui/controllers/api/mrfrake_controller.h` → `${CLIENT_ROOT_DIR}/ui/controllers/api/fblink_controller.h`
- `${CLIENT_ROOT_DIR}/ui/controllers/api/mrfrake_controller.cpp` → `${CLIENT_ROOT_DIR}/ui/controllers/api/fblink_controller.cpp`

- [ ] **Step 2: Обновить корневой CMakeLists.txt**

Заменить в `CMakeLists.txt` (replace_all):
- `set(PROJECT Mr.Frake.VPN)` → `set(PROJECT FBLink.VPN)`
- `DESCRIPTION "Mr.Frake-VPN"` → `DESCRIPTION "FBLink-VPN"`
- `set(CPACK_PACKAGE_NAME "MrFrakeVPN")` → `set(CPACK_PACKAGE_NAME "FBLinkVPN")`
- `set(CPACK_PACKAGE_VENDOR "MrFrakeVPN")` → `set(CPACK_PACKAGE_VENDOR "FBLink")`
- `set(CPACK_PACKAGE_INSTALL_DIRECTORY "MrFrakeVPN")` → `set(CPACK_PACKAGE_INSTALL_DIRECTORY "FBLinkVPN")`
- `set(CPACK_PACKAGE_EXECUTABLES "MrFrakeVPN" "MrFrakeVPN")` → `set(CPACK_PACKAGE_EXECUTABLES "FBLinkVPN" "FBLinkVPN")`

- [ ] **Step 3: Commit**

```bash
git add client/cmake/sources.cmake CMakeLists.txt
git commit -m "refactor: update CMake build files for FBLink rebrand"
```

---

### Task 3: Обновить C++ контроллеры (core + connection)

**Files:**
- Modify: `client/core/controllers/coreController.h`
- Modify: `client/core/controllers/coreController.cpp`
- Modify: `client/ui/controllers/connectionController.cpp`

- [ ] **Step 1: Обновить coreController.h**

Заменить в `client/core/controllers/coreController.h` (replace_all):
- `#include "ui/controllers/api/mrfrake_controller.h"` → `#include "ui/controllers/api/fblink_controller.h"`
- `QScopedPointer<MrFrakeController> m_drFrakeController;` → `QScopedPointer<FBLinkController> m_fbLinkController;`

- [ ] **Step 2: Обновить coreController.cpp — символы**

Заменить в `client/core/controllers/coreController.cpp` (replace_all):
- `m_drFrakeController` → `m_fbLinkController`
- `new MrFrakeController(` → `new FBLinkController(`
- `setContextProperty("MrFrakeController",` → `setContextProperty("FBLinkController",`

- [ ] **Step 3: Обновить coreController.cpp — комментарии**

Заменить в `client/core/controllers/coreController.cpp` (replace_all):
- `Mr.Frake VPN` → `FBLink VPN` (в любых комментариях)
- `MrFrake` → `FBLink` (в любых комментариях)

- [ ] **Step 4: Обновить комментарий в connectionController.cpp**

Заменить в `client/ui/controllers/connectionController.cpp` строку 38:
- `// Check for AmneziaVPN-service OR the service named after the running EXE (e.g. MrFrakeVPN-service)` → `// Check for AmneziaVPN-service OR the service named after the running EXE (e.g. FBLinkVPN-service)`

- [ ] **Step 5: Commit**

```bash
git add client/core/controllers/coreController.h \
        client/core/controllers/coreController.cpp \
        client/ui/controllers/connectionController.cpp
git commit -m "refactor: update C++ controllers for FBLink rebrand"
```

---

### Task 4: Обновить pageController (enum страниц и QSettings)

**Files:**
- Modify: `client/ui/controllers/pageController.h`
- Modify: `client/ui/controllers/pageController.cpp`

- [ ] **Step 1: Обновить pageController.h**

Заменить в `client/ui/controllers/pageController.h` (replace_all):
- `PageMrFrakeLogin,` → `PageFBLinkLogin,`
- `PageMrFrakeRegister,` → `PageFBLinkRegister,`
- `PageMrFrakeSubscription,` → `PageFBLinkSubscription,`

- [ ] **Step 2: Обновить pageController.cpp — QSettings**

Найти в `client/ui/controllers/pageController.cpp` строку:
```cpp
QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "MrFrakeVPN", "MrFrakeVPN");
```
Заменить (оба аргумента-строки) на:
```cpp
QSettings qSettings(QSettings::NativeFormat, QSettings::UserScope, "FBLinkVPN", "FBLinkVPN");
```

- [ ] **Step 3: Commit**

```bash
git add client/ui/controllers/pageController.h \
        client/ui/controllers/pageController.cpp
git commit -m "refactor: rename page enum values and QSettings key for FBLink rebrand"
```

---

## Chunk 2: QML страницы и компоненты

### Task 5: Переименовать QML страницы и обновить содержимое

**Files:**
- Rename: `client/ui/qml/Pages2/PageMrFrakeLogin.qml` → `PageFBLinkLogin.qml`
- Rename: `client/ui/qml/Pages2/PageMrFrakeRegister.qml` → `PageFBLinkRegister.qml`
- Rename: `client/ui/qml/Pages2/PageMrFrakeSubscription.qml` → `PageFBLinkSubscription.qml`
- Modify: `client/resources.qrc`

- [ ] **Step 1: Переименовать QML файлы**

```bash
mv client/ui/qml/Pages2/PageMrFrakeLogin.qml       client/ui/qml/Pages2/PageFBLinkLogin.qml
mv client/ui/qml/Pages2/PageMrFrakeRegister.qml    client/ui/qml/Pages2/PageFBLinkRegister.qml
mv client/ui/qml/Pages2/PageMrFrakeSubscription.qml client/ui/qml/Pages2/PageFBLinkSubscription.qml
```

- [ ] **Step 2: Обновить PageFBLinkLogin.qml**

Заменить все вхождения (replace_all):
- `MrFrakeController` → `FBLinkController`
- `PageMrFrakeRegister` → `PageFBLinkRegister`
- `PageMrFrakeSubscription` → `PageFBLinkSubscription`
- `PageMrFrakeLogin` → `PageFBLinkLogin`

- [ ] **Step 3: Обновить PageFBLinkRegister.qml**

Заменить все вхождения (replace_all):
- `MrFrakeController` → `FBLinkController`
- `PageMrFrakeLogin` → `PageFBLinkLogin`
- `PageMrFrakeRegister` → `PageFBLinkRegister`

- [ ] **Step 4: Обновить PageFBLinkSubscription.qml — символы**

Заменить все вхождения (replace_all):
- `MrFrakeController` → `FBLinkController`
- `PageMrFrakeLogin` → `PageFBLinkLogin`
- `PageMrFrakeSubscription` → `PageFBLinkSubscription`

- [ ] **Step 5: Обновить PageFBLinkSubscription.qml — контактные URL**

Заменить конкретные строки (плейсхолдеры — обновить реальными значениями перед деплоем):
- `"https://mrfrake.ru/terms"` → `"https://fblink.com/terms"`
- `"https://mrfrake.ru/privacy"` → `"https://fblink.com/privacy"`

- [ ] **Step 6: Обновить resources.qrc**

Заменить в `client/resources.qrc` (replace_all):
- `ui/qml/Pages2/PageMrFrakeLogin.qml` → `ui/qml/Pages2/PageFBLinkLogin.qml`
- `ui/qml/Pages2/PageMrFrakeRegister.qml` → `ui/qml/Pages2/PageFBLinkRegister.qml`
- `ui/qml/Pages2/PageMrFrakeSubscription.qml` → `ui/qml/Pages2/PageFBLinkSubscription.qml`

- [ ] **Step 7: Commit**

```bash
git add client/ui/qml/Pages2/PageFBLinkLogin.qml \
        client/ui/qml/Pages2/PageFBLinkRegister.qml \
        client/ui/qml/Pages2/PageFBLinkSubscription.qml \
        client/resources.qrc
git rm client/ui/qml/Pages2/PageMrFrakeLogin.qml \
       client/ui/qml/Pages2/PageMrFrakeRegister.qml \
       client/ui/qml/Pages2/PageMrFrakeSubscription.qml
git commit -m "refactor: rename QML pages for FBLink rebrand"
```

---

### Task 6: Обновить остальные QML файлы

**Files:**
- Modify: `client/ui/qml/Components/ConnectButton.qml`
- Modify: `client/ui/qml/Components/SubscriptionBanner.qml`
- Modify: `client/ui/qml/Pages2/PageHome.qml`
- Modify: `client/ui/qml/Pages2/PageStart.qml`
- Modify: `client/ui/qml/Pages2/PageSetupWizardStart.qml`
- Modify: `client/ui/qml/Pages2/PageSetupWizardConfigSource.qml`
- Modify: `client/ui/qml/Pages2/PageSettings.qml`
- Modify: `client/ui/qml/Pages2/PageSettingsAbout.qml`
- Modify: `client/ui/qml/Pages2/PageSettingsBackup.qml`
- Modify: `client/ui/qml/Pages2/PageSettingsKillSwitchExceptions.qml`
- Modify: `client/ui/qml/Pages2/PageSettingsLogging.qml`
- Modify: `client/ui/qml/Pages2/PageSettingsSplitTunneling.qml`
- Modify: `client/ui/qml/Pages2/PageShare.qml`
- Modify: `client/ui/qml/Pages2/PageShareConnection.qml`
- Modify: `client/ui/qml/Pages2/PageShareFullAccess.qml`
- Modify: `client/ui/qml/main2.qml`

- [ ] **Step 1: ConnectButton.qml**

Заменить все вхождения (replace_all):
- `MrFrakeController` → `FBLinkController`
- `PageMrFrakeSubscription` → `PageFBLinkSubscription`

- [ ] **Step 2: SubscriptionBanner.qml**

Заменить все вхождения (replace_all):
- `MrFrakeController` → `FBLinkController`
- `PageMrFrakeSubscription` → `PageFBLinkSubscription`
- `PageMrFrakeLogin` → `PageFBLinkLogin`

- [ ] **Step 3: PageHome.qml**

Заменить все вхождения (replace_all):
- `MrFrakeController` → `FBLinkController`
- `PageMrFrakeSubscription` → `PageFBLinkSubscription`
- `PageMrFrakeLogin` → `PageFBLinkLogin`

- [ ] **Step 4: PageStart.qml**

Заменить все вхождения (replace_all):
- `MrFrakeController` → `FBLinkController`
- `PageMrFrakeLogin` → `PageFBLinkLogin`
- `PageMrFrakeSubscription` → `PageFBLinkSubscription`

- [ ] **Step 5: PageSetupWizardStart.qml**

Заменить:
- `PageMrFrakeLogin` → `PageFBLinkLogin`

- [ ] **Step 6: PageSetupWizardConfigSource.qml**

Заменить все вхождения (replace_all):
- `"MrFrakeVPN.log"` → `"FBLinkVPN.log"`
- `"/MrFrakeVPN"` → `"/FBLinkVPN"`

- [ ] **Step 7: PageSettings.qml**

Заменить все вхождения (replace_all):
- `MrFrakeController` → `FBLinkController`

- [ ] **Step 8: PageSettingsAbout.qml — символы и контактные данные**

Заменить все вхождения (replace_all):
- `MrFrakeController` → `FBLinkController`

Заменить контактные данные (плейсхолдеры — обновить реальными значениями перед деплоем):
- `"https://t.me/mrfrake_vpn"` → `"https://t.me/fblink_vpn"`
- `"support@mrfrake.vpn"` → `"support@fblink.com"`
- `Qt.openUrlExternally("mailto:support@mrfrake.vpn")` → `Qt.openUrlExternally("mailto:support@fblink.com")`

- [ ] **Step 9: PageSettingsBackup.qml**

Заменить все вхождения (replace_all):
- `"MrFrakeVPN.backup"` → `"FBLinkVPN.backup"`
- `"/MrFrakeVPN"` → `"/FBLinkVPN"`

- [ ] **Step 10: PageSettingsKillSwitchExceptions.qml**

Заменить все вхождения (replace_all):
- `"mrfrake_killswitch_exceptions.json"` → `"fblink_killswitch_exceptions.json"`
- `"/mrfrake_killswitch_exceptions"` → `"/fblink_killswitch_exceptions"`

- [ ] **Step 11: PageSettingsLogging.qml**

Заменить все вхождения (replace_all):
- `"MrFrakeVPN.log"` → `"FBLinkVPN.log"`
- `"/MrFrakeVPN"` → `"/FBLinkVPN"`

- [ ] **Step 12: PageSettingsSplitTunneling.qml**

Заменить все вхождения (replace_all):
- `"mrfrake_sites.json"` → `"fblink_sites.json"`
- `"/mrfrake_sites"` → `"/fblink_sites"`

- [ ] **Step 13: PageShare.qml**

Заменить все вхождения (replace_all):
- `"mrfrake_config"` → `"fblink_config"`
- `"mrfrake_for_openvpn"` → `"fblink_for_openvpn"`
- `"mrfrake_for_wireguard"` → `"fblink_for_wireguard"`
- `"mrfrake_for_awg"` → `"fblink_for_awg"`
- `"mrfrake_for_shadowsocks"` → `"fblink_for_shadowsocks"`
- `"mrfrake_for_cloak"` → `"fblink_for_cloak"`
- `"mrfrake_for_xray"` → `"fblink_for_xray"`

- [ ] **Step 14: PageShareConnection.qml**

Заменить все вхождения (replace_all):
- `"mrfrake_config"` → `"fblink_config"`

- [ ] **Step 15: PageShareFullAccess.qml**

Заменить все вхождения (replace_all):
- `"mrfrake_config"` → `"fblink_config"`

- [ ] **Step 16: main2.qml — заголовок окна**

Заменить:
- `title: "Mr.Frake VPN"` → `title: "FBLink VPN"`

- [ ] **Step 17: Commit**

```bash
git add client/ui/qml/Components/ConnectButton.qml \
        client/ui/qml/Components/SubscriptionBanner.qml \
        client/ui/qml/Pages2/PageHome.qml \
        client/ui/qml/Pages2/PageStart.qml \
        client/ui/qml/Pages2/PageSetupWizardStart.qml \
        client/ui/qml/Pages2/PageSetupWizardConfigSource.qml \
        client/ui/qml/Pages2/PageSettings.qml \
        client/ui/qml/Pages2/PageSettingsAbout.qml \
        client/ui/qml/Pages2/PageSettingsBackup.qml \
        client/ui/qml/Pages2/PageSettingsKillSwitchExceptions.qml \
        client/ui/qml/Pages2/PageSettingsLogging.qml \
        client/ui/qml/Pages2/PageSettingsSplitTunneling.qml \
        client/ui/qml/Pages2/PageShare.qml \
        client/ui/qml/Pages2/PageShareConnection.qml \
        client/ui/qml/Pages2/PageShareFullAccess.qml \
        client/ui/qml/main2.qml
git commit -m "refactor: update all QML files for FBLink rebrand"
```

---

## Chunk 3: Installer, сборка, CI/CD

### Task 7: Переименовать installer-пакет и обновить всё содержимое

**Files:**
- Rename dir: `deploy/installer/packages/org.mrfrakevpn.package/` → `deploy/installer/packages/org.fblinkvpn.package/`
- Modify: `deploy/installer/packages/org.fblinkvpn.package/meta/package.xml.in`
- Modify: `deploy/installer/packages/org.fblinkvpn.package/meta/componentscript.js`
- Modify: `deploy/installer/config/windows.xml.in`
- Modify: `deploy/installer/config.cmake`
- Modify: `deploy/installer/wix/close_client_patch.xml`
- Modify: `deploy/installer/wix/service_install_patch.xml`

- [ ] **Step 1: Переименовать директорию пакета**

```bash
mv deploy/installer/packages/org.mrfrakevpn.package \
   deploy/installer/packages/org.fblinkvpn.package
```

- [ ] **Step 2: Обновить package.xml.in**

Заменить в `deploy/installer/packages/org.fblinkvpn.package/meta/package.xml.in`:
- `<DisplayName>Mr.Frake VPN</DisplayName>` → `<DisplayName>FBLink VPN</DisplayName>`
- `<Description>Installation package for Mr.Frake VPN</Description>` → `<Description>Installation package for FBLink VPN</Description>`

- [ ] **Step 3: Обновить componentscript.js**

Заменить в `deploy/installer/packages/org.fblinkvpn.package/meta/componentscript.js`:
- `// The actual file on disk is named after the app (MrFrakeVPN-service.exe),` → `// The actual file on disk is named after the app (FBLinkVPN-service.exe),`

  Примечание: внутреннее имя сервиса `AmneziaVPN-service` в строке ниже **не меняется** — оно жёстко задано в скомпилированном бинарнике сервиса.

- [ ] **Step 4: Обновить windows.xml.in**

Заменить в `deploy/installer/config/windows.xml.in`:
- `<Name>MrFrakeVPN</Name>` → `<Name>FBLinkVPN</Name>`

- [ ] **Step 5: Обновить config.cmake**

Заменить в `deploy/installer/config.cmake` (два вхождения):
- `packages/org.mrfrakevpn.package/meta/package.xml.in` → `packages/org.fblinkvpn.package/meta/package.xml.in`
- `packages/org.mrfrakevpn.package/meta/package.xml` → `packages/org.fblinkvpn.package/meta/package.xml`

- [ ] **Step 6: Обновить close_client_patch.xml**

Заменить в `deploy/installer/wix/close_client_patch.xml`:
- `Id="CloseMrFrakeClient"` → `Id="CloseFBLinkClient"`
- `Description="Closing Mr.Frake VPN client"` → `Description="Closing FBLink VPN client"`

- [ ] **Step 7: Обновить service_install_patch.xml**

Заменить в `deploy/installer/wix/service_install_patch.xml`:
- `Id="MrFrakeServiceInstall"` → `Id="FBLinkServiceInstall"`
- `Id="MrFrakeServiceControl"` → `Id="FBLinkServiceControl"`
- `DisplayName="Mr.Frake VPN Service"` → `DisplayName="FBLink VPN Service"`
- `Description="Service for Mr.Frake VPN"` → `Description="Service for FBLink VPN"`

  Примечание: атрибут `Name="AmneziaVPN-service"` **не меняется** — это внутреннее имя сервиса.

- [ ] **Step 8: Commit**

```bash
git add deploy/installer/packages/org.fblinkvpn.package \
        deploy/installer/config/windows.xml.in \
        deploy/installer/config.cmake \
        deploy/installer/wix/close_client_patch.xml \
        deploy/installer/wix/service_install_patch.xml
git rm -r deploy/installer/packages/org.mrfrakevpn.package 2>/dev/null || true
git commit -m "refactor: update installer package for FBLink rebrand"
```

---

### Task 8: Обновить сборочный скрипт Windows

**Files:**
- Modify: `deploy/build_windows.bat`

- [ ] **Step 1: Обновить переменные APP_NAME и APP_DOMAIN**

Заменить в `deploy/build_windows.bat`:
- `set APP_NAME=MrFrakeVPN` → `set APP_NAME=FBLinkVPN`
- `set APP_DOMAIN=org.mrfrakevpn.package` → `set APP_DOMAIN=org.fblinkvpn.package`

- [ ] **Step 2: Обновить хардкоженное имя иконки (строка 86)**

Строка содержит **хардкоженный литерал** (не переменную `%APP_NAME%`):
```bat
copy /Y "%PROJECT_DIR%\client\images\app.ico" "%OUT_APP_DIR%\MrFrakeVPN.ico" >nul
```
Заменить на:
```bat
copy /Y "%PROJECT_DIR%\client\images\app.ico" "%OUT_APP_DIR%\FBLinkVPN.ico" >nul
```
Примечание: исходный файл `app.ico` не переименовывается — меняется только имя скопированного файла в выходной директории.

- [ ] **Step 3: Commit**

```bash
git add deploy/build_windows.bat
git commit -m "refactor: update build_windows.bat for FBLink rebrand"
```

---

### Task 9: Обновить admin-панель бэкенда

**Files:**
- Modify: `vpn-backend/admin/index.html`

- [ ] **Step 1: Обновить заголовок и отображаемое имя**

Заменить в `vpn-backend/admin/index.html`:
- `<title>Mr.Frake VPN — Admin Panel</title>` → `<title>FBLink VPN — Admin Panel</title>`
- `<h1>⚡ Mr.Frake VPN</h1>` → `<h1>⚡ FBLink VPN</h1>`
- `<div class="logo">Mr.Frake <span>VPN</span></div>` → `<div class="logo">FBLink <span>VPN</span></div>`

- [ ] **Step 2: Обновить email-плейсхолдер и localStorage ключи**

Заменить в `vpn-backend/admin/index.html` (replace_all):
- `placeholder="admin@mrfrake.vpn"` → `placeholder="admin@fblink.com"`
- `'mrfrake_admin_token'` → `'fblink_admin_token'` (4 вхождения: getItem, setItem, removeItem×2)

- [ ] **Step 3: Commit**

```bash
git add vpn-backend/admin/index.html
git commit -m "refactor: update admin panel for FBLink rebrand"
```

---

### Task 10: Обновить GitHub Actions workflow

**Files:**
- Modify: `.github/workflows/deploy.yml`

- [ ] **Step 1: Обновить deploy.yml**

Заменить в `.github/workflows/deploy.yml` (replace_all):
- `MrFrakeVPN` → `FBLinkVPN`

- [ ] **Step 2: Commit**

```bash
git add .github/workflows/deploy.yml
git commit -m "refactor: update CI/CD artifact names for FBLink rebrand"
```

---

## Chunk 4: Финальная проверка

### Task 11: Проверить полноту замен и обновить документацию

- [ ] **Step 1: Финальный grep — убедиться что старые имена не остались**

```bash
grep -r "MrFrake\|mrfrake\|DrFrake\|drfrake\|Mr\.Frake\|Dr\.Frake\|mrfrakevpn\|drfrakevpn" \
  client/ deploy/ CMakeLists.txt .github/ \
  --include="*.h" --include="*.cpp" --include="*.qml" \
  --include="*.cmake" --include="*.bat" --include="*.yml" \
  --include="*.xml" --include="*.xml.in" --include="*.js" \
  --include="*.qrc" --include="*.md" \
  -l
```

Ожидаемый результат: **пустой вывод**. Если есть файлы — исправить перед продолжением.

- [ ] **Step 2: Обновить CLAUDE.md**

Заменить в `CLAUDE.md` (replace_all):
- `**Mr.Frake VPN**` → `**FBLink VPN**`
- `MrFrakeVPN` → `FBLinkVPN`
- `mrfrake_controller` → `fblink_controller`
- `MrFrake-специфичные` → `FBLink-специфичные`
- `MrFrakeController` → `FBLinkController`

- [ ] **Step 3: Commit документации**

```bash
git add CLAUDE.md
git commit -m "docs: update CLAUDE.md for FBLink rebrand"
```

- [ ] **Step 4: Проверить сборку**

Открыть проект в Qt Creator и запустить Build, или:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
```

Убедиться что сборка проходит без ошибок, связанных с переименованием.

---

## Сводная таблица изменений

| Файл | Тип | Что меняется |
|------|-----|-------------|
| `client/ui/controllers/api/mrfrake_controller.h` | Rename → `fblink_controller.h` | Header guard, class name, method name |
| `client/ui/controllers/api/mrfrake_controller.cpp` | Rename → `fblink_controller.cpp` | Include, class/method/var names, QSettings key (оба аргумента), server-matching strings |
| `client/cmake/sources.cmake` | Edit | 2 вхождения пути к файлам контроллера |
| `CMakeLists.txt` | Edit | PROJECT, DESCRIPTION, 4× CPACK_PACKAGE_* |
| `client/core/controllers/coreController.h` | Edit | Include, тип члена, имя члена |
| `client/core/controllers/coreController.cpp` | Edit | Создание контроллера, context property, все обращения к члену, комментарии |
| `client/ui/controllers/connectionController.cpp` | Edit | Комментарий со старым именем |
| `client/ui/controllers/pageController.h` | Edit | 3 enum значения |
| `client/ui/controllers/pageController.cpp` | Edit | QSettings оба аргумента `"MrFrakeVPN"` → `"FBLinkVPN"` |
| `client/ui/qml/Pages2/PageMrFrakeLogin.qml` | Rename → `PageFBLinkLogin.qml` | MrFrakeController → FBLinkController, page refs |
| `client/ui/qml/Pages2/PageMrFrakeRegister.qml` | Rename → `PageFBLinkRegister.qml` | MrFrakeController → FBLinkController, page refs |
| `client/ui/qml/Pages2/PageMrFrakeSubscription.qml` | Rename → `PageFBLinkSubscription.qml` | MrFrakeController → FBLinkController, page refs, URLs |
| `client/resources.qrc` | Edit | 3 пути QML файлов |
| `client/ui/qml/Components/ConnectButton.qml` | Edit | MrFrakeController, PageMrFrakeSubscription |
| `client/ui/qml/Components/SubscriptionBanner.qml` | Edit | MrFrakeController, page enum refs |
| `client/ui/qml/Pages2/PageHome.qml` | Edit | MrFrakeController, PageEnum refs |
| `client/ui/qml/Pages2/PageStart.qml` | Edit | MrFrakeController, PageEnum refs |
| `client/ui/qml/Pages2/PageSetupWizardStart.qml` | Edit | PageEnum ref |
| `client/ui/qml/Pages2/PageSetupWizardConfigSource.qml` | Edit | Имена файлов/папок (log) |
| `client/ui/qml/Pages2/PageSettings.qml` | Edit | MrFrakeController |
| `client/ui/qml/Pages2/PageSettingsAbout.qml` | Edit | MrFrakeController, URLs/email (плейсхолдеры) |
| `client/ui/qml/Pages2/PageSettingsBackup.qml` | Edit | Имена файлов/папок (backup) |
| `client/ui/qml/Pages2/PageSettingsKillSwitchExceptions.qml` | Edit | Имена файлов/папок (killswitch json) |
| `client/ui/qml/Pages2/PageSettingsLogging.qml` | Edit | Имена файлов/папок (log) |
| `client/ui/qml/Pages2/PageSettingsSplitTunneling.qml` | Edit | Имена файлов/папок (sites json) |
| `client/ui/qml/Pages2/PageShare.qml` | Edit | 7 имён файлов конфигов |
| `client/ui/qml/Pages2/PageShareConnection.qml` | Edit | configFileName default |
| `client/ui/qml/Pages2/PageShareFullAccess.qml` | Edit | configFileName literal |
| `client/ui/qml/main2.qml` | Edit | Window title |
| `deploy/installer/packages/org.mrfrakevpn.package/` | Rename dir → `org.fblinkvpn.package/` | — |
| `deploy/installer/packages/.../package.xml.in` | Edit | DisplayName, Description |
| `deploy/installer/packages/.../componentscript.js` | Edit | Комментарий с именем exe |
| `deploy/installer/config/windows.xml.in` | Edit | `<Name>` |
| `deploy/installer/config.cmake` | Edit | 2 пути к package.xml |
| `deploy/installer/wix/close_client_patch.xml` | Edit | Id атрибут |
| `deploy/installer/wix/service_install_patch.xml` | Edit | 2× Id, DisplayName, Description |
| `deploy/build_windows.bat` | Edit | APP_NAME, APP_DOMAIN, хардкоженное имя иконки |
| `.github/workflows/deploy.yml` | Edit | Artifact names |
| `vpn-backend/admin/index.html` | Edit | Title, logo text, email placeholder, localStorage key |
| `CLAUDE.md` | Edit | Документация |
