# QR pairing — аудит и инвентарь PR

**Дата:** 2026-05-19  
**PR:** [amnezia-vpn/amnezia-client#2580](https://github.com/amnezia-vpn/amnezia-client/pull/2580) — *Feat: Implement qr code generation and scanning*  
**Ветка:** `feat/implement-QR-code-generation-and-scanning` → **`dev`**  
**Объём PR (GitHub / `git diff origin/dev...HEAD`):** 68 файлов, **+6641 / −169** строк  

**База для инвентаря «что добавил автор»:** `origin/dev...HEAD` (не первый коммит `2f6714e` и не сравнение с `main`).

---

## Что НЕ входит в PR #2580 (не удалять при чистке pairing)

Эти пути **отсутствуют** в diff PR против `dev`. Попадали в старый аудит из‑за сравнения с `2f6714e` или локального merge — **это прилетело с `dev`, не ваш QR-diff:**

| Путь | Примечание |
|------|------------|
| `client/server_scripts/serverScripts.qrc` (+ telemt, mtproxy) | Контейнеры с ветки **dev** — **оставить** |
| `client/client_scripts/clientScripts.qrc` (−`linux_installer.sh`) | Не в PR #2580 |

При ревью pairing **не предлагать** откат telemt/mtproxy в `server_scripts.qrc`.

---

## Продукт: что делает фича

| Сценарий | UI | Gateway API |
|----------|-----|-------------|
| **Receive (TV / второе устройство)** | `PageSetupWizardApiQrPairingReceive` (+ wizard `PageSetupWizardConfigSource`) | `POST v1/generate_qr` (long-poll), импорт конфига |
| **Send (телефон с Premium)** | `PageSettingsApiQrPairingSend` (вход с `PageSettingsApiDevices`) | `POST v1/scan_qr` после скана session UUID |
| **Desktop** | Текст «доступно в mobile app» | — |

**Сканирование на prod Send:**

| Платформа | Реализация |
|-----------|------------|
| iOS | `iosPairingQrOverlayWindow` (native UIWindow) |
| Android | `CameraActivity` + `openPairingQrScanner()` |
| Desktop | Без камеры |

---

## Инвентарь PR #2580 (68 файлов)

Легенда: **A** = новый файл, **M** = изменён существующий.

### Корень

| | Файл |
|---|------|
| M | `CMakeLists.txt` |

### Core — API pairing

| | Файл | Назначение |
|---|------|------------|
| A | `client/core/controllers/api/pairingController.{h,cpp}` | Payload/parse `generate_qr`, `scan_qr` |
| A | `client/ui/controllers/api/pairingUiController.{h,cpp}` | QML API, TV/phone сессии, камера, gateway |
| M | `client/core/controllers/api/subscriptionController.{h,cpp}` | `importServerFromQrPairingResponse` |
| M | `client/core/controllers/gatewayController.{h,cpp}` | `postAsync` keepAlive, dev proxy/plaintext |
| M | `client/core/controllers/updateController.cpp` | Пропуск updater на dev AGW |
| M | `client/core/controllers/coreController.{h,cpp}`, `coreSignalHandlers.cpp` | Регистрация `PairingUiController` |
| M | `client/core/controllers/api/newsController.cpp` | (смежная правка в ветке) |
| M | `client/core/repositories/secureAppSettingsRepository.cpp` | Dev gateway endpoint |
| M | `client/core/utils/qrCodeUtils.{h,cpp}` | `generateQrCodeImageSeriesPlainText` (UUID в QR) |
| M | `client/core/utils/errorCodes.h`, `errorStrings.cpp` | Коды 1117–1122 pairing |
| M | `client/core/utils/api/apiUtils.{h,cpp}` | HTTP → pairing error codes |
| M | `client/core/utils/constants/apiKeys.h` | Ключи metadata |
| M | `client/core/utils/networkUtilities.{h,cpp}` | (смежно) |
| M | `client/amneziaApplication.cpp` | (смежно) |

### UI — QML

| | Файл | Назначение |
|---|------|------------|
| A | `PageSettingsApiQrPairingSend.qml` | Prod Send |
| A | `PageSetupWizardApiQrPairingReceive.qml` | Prod Receive |
| A | `PageSettingsApiQrPairingDev.qml` | Dev: TV+phone на одной странице |
| M | `PageSettingsApiDevices.qml` | Кнопка → Send |
| M | `PageSetupWizardConfigSource.qml` | Wizard → Receive, `canOpenTvQrPairingPage` |
| M | `PageDevMenu.qml` | Пункт Dev → pairing (если Dev-страница в PR) |
| M | `PageStart.qml`, `main2.qml`, `TabImageButtonType.qml`, `StackViewType.qml` | Chrome/навигация (часть — под embedded, см. уровень 1) |
| M | `client/ui/qml/qml.qrc` | Регистрация страниц |
| M | `pageController.h` | Enum страниц pairing |
| M | `subscriptionUiController.cpp`, `apiAccountInfoModel.{h,cpp}` | Устройства / аккаунт после pairing |

### iOS

| | Файл | Назначение |
|---|------|------------|
| A | `iosPairingQrOverlayWindow.{h,mm}` | Prod scanner overlay |
| A | `iosPairingCameraAccess.{h,mm,stub}` | Permissions + embedded underlay (legacy/Dev) |
| M | `QRCodeReaderBase.{h,cpp,mm}` | Torch, relayout; wizard reader |

### Android

| | Файл | Назначение |
|---|------|------------|
| A | `PairingQrScanGeometry.kt`, `PairingQrScanBracketPaths.kt`, `PairingQrScanOverlayView.kt` | Рамка «дырки» в превью |
| A | `PairingQrEmbeddedCamera.kt` | Embedded камера в Qt (кандидат на удаление в уровне 1) |
| A | `ic_pairing_back.xml`, `torch_fab_bg.xml` | UI CameraActivity |
| M | `CameraActivity.kt`, `camera_preview.xml` | Prod fullscreen scan |
| M | `AmneziaActivity.kt`, `QtAndroidController.kt` | `startPairingQrCodeReader`, JNI callbacks |
| M | `android_controller.{h,cpp}` | JNI |
| M | `values/strings.xml`, `values-ru/strings.xml` | Строки pairing |

### Сборка и тесты

| | Файл |
|---|------|
| M | `client/cmake/sources.cmake`, `ios.cmake`, `macos_ne.cmake` |
| A | `client/tests/testPairingParsers.cpp` |
| M | `client/tests/CMakeLists.txt` |
| M | `client/translations/amneziavpn_ru_RU.ts` |

### Удалено в истории PR (не в финальном diff как добавление)

- Старая `PageSettingsApiQrPairing.qml` (коммит `b46a9e38` remove old file)
- `AMNEZIA_QR_PAIRING_ALLOW` и mock (коммит `d8668742`)
- `tvPairingUiPhase`, `amneziaIosPairingQrOverlayIsPresented()` (локальная чистка)

---

## Архитектура: что лишнее на prod Send (после упрощения)

На **текущем упрощённом Send** (локально ~361 строка) дубли из **исходного PR** не участвуют:

| Платформа | Рабочий путь | Не используется на prod Send |
|-----------|--------------|------------------------------|
| iOS | Overlay | Embedded `QRCodeReader`, `scanStep`, underlay |
| Android | `CameraActivity` | `PairingQrEmbeddedCamera`, embedded QML |
| Desktop | Текст | Scanner UI |

Это **кандидаты на удаление в уровне 1**, не «ошибка PR» — эволюция к одному scanner на платформу.

---

## Аудит: оставить / убрать / под вопросом

### Ядро PR — не удалять

`pairingController`, `pairingUiController`, Send/Receive QML, `iosPairingQrOverlayWindow`, Android `CameraActivity` + geometry/overlay, `gatewayController` keepAlive, `importServerFromQrPairingResponse`, error codes, `testPairingParsers`, `qrCodeUtils` PlainText, навигация Devices/ConfigSource.

### Кандидаты на удаление (уровень 1 или follow-up)

| Объект | В PR #2580 | Уровень 1 / примечание |
|--------|------------|-------------------------|
| `PageSettingsApiQrPairingDev.qml` + Dev menu | **A** | R8 — убрать из релиза (локально **D**, не закоммичено) |
| `PairingQrEmbeddedCamera.kt` + JNI embedded | **A** | R9 — локально **D** |
| `pairingQrChromeDebug`, embedded chrome в `main2`/`PageStart`/`TabImageButton` | **M** | R1, R10 — локально убрано |
| Verbose `qInfo` / `NSLog` | **M** | R2–R4 |
| `scanStep` / `QRCodeReader` в Send (если ещё есть в remote) | в ранних версиях Send | R11 — локально убрано |
| `tryDecodeLegacyChunkedPairingQrPayload` | в `pairingUiController` | **P1** — продуктовое решение |
| `canOpenTvQrPairingPage` → `v1/news` | в `pairingUiController` | **P2** |

### Не трогать (не из PR pairing)

- `server_scripts.qrc` telemt/mtproxy — **dev**, см. таблицу выше.

---

## Уровень 1 (R1–R12)

Цель: prod Send/Receive без Dev/embedded/debug; **без отката telemt/mtproxy**.

| ID | Задача | PR #2580 | Локально |
|----|--------|----------|----------|
| **R1** | `pairingQrChromeDebug` + debug-цвета | M Send/Start | ✅ |
| **R2** | Урезать `qInfo` `[PairingUi]` | M | ✅ |
| **R3** | Урезать `NSLog` `[PairingQrOverlay]` | A overlay | ✅ (10 строк ошибок) |
| **R4** | Урезать `NSLog` `[PairingCamera]` | A camera access | ✅ (1 строка: no root VC) |
| **R5** | ~~Откат `client_scripts.qrc`~~ | **N/A — не в PR** | — |
| **R6** | ~~Откат `server_scripts` telemt/mtproxy~~ | **N/A — не в PR, не удалять** | — |
| **R7** | Нет `PageSettingsApiQrPairing` / enum | удалено в PR | ✅ |
| **R8** | Dev-страница + menu + enum + qrc | **A** Dev.qml | ✅ удалено |
| **R9** | `PairingQrEmbeddedCamera` + JNI | **A** | ✅ удалено |
| **R10** | embedded chrome main2/PageStart/TabImage | M | ✅ |
| **R11** | Упростить Send (overlay/Activity only) | A Send | ✅ ~361 строк |
| **R12** | Убрать API embedded из `pairingUiController` | A | ✅ |
| **R13** | Срезать `iosPairingCameraAccess` до permissions | A | ✅ 2026-05-19 |
| **R14** | `QRCodeReaderBase`: убрать relayout underlay | M | ✅ |
| **R15** | `StackViewType`: убрать `clip: false` (embedded dim) | M | ✅ |
| **R16** | Дубль `getAccountInfo` в Devices `onPhonePairingSucceeded` | M | ✅ |
| **P1** | `tryDecodeLegacyChunkedPairingQrPayload` | — | ✅ удалено |
| **R17** | `QRCodeReader::setTorchEnabled` + torch в `QRCodeReaderBase.mm` | M | ✅ 2026-05-19 |
| **R18** | `PairingController::gatewayStringMetadataArray` + тесты | — | ✅ |
| **R19** | `NetworkUtilities::hostIsPrivateLanAddress` | M | ✅ |
| **R20** | Q_PROPERTY `tvSessionUuid`/`tvPairingBusy`/`tvStatusMessage`/`phoneStatusMessage`; `iosNative*`/`androidNative*` build flags | M | ✅ |
| **R21** | `ApiAccountInfoModel::AvailableDeviceSlotsRole` | M | ✅ |

**Уровень 1 + R17–R21 закрыты** (логи R2–R4 — отдельный проход).

---

## P2–P6 (опционально)

| ID | Тема |
|----|------|
| ~~P1~~ | ~~`tryDecodeLegacyChunkedPairingQrPayload`~~ — **удалено** |
| P2 | Probe `canOpenTvQrPairingPage` → `POST v1/news` |
| P3 | Упростить `iosNativePairingQrOverlayBuild` / `androidNativePairingQrOverlayBuild` |
| P4 | `iosPairingCameraAccess` — оставить только для `QRCodeReaderBase` / wizard |
| P5 | `tvStatusMessage` / `tvPairingBusy` только для Dev или убрать |
| P6 | Сократить `iosPairingQrOverlayWindow.mm` после стабилизации UI |

---

## Статус

| Элемент | Статус |
|---------|--------|
| Документ привязан к **PR #2580 vs dev** | ✅ 2026-05-19 |
| telemt/mtproxy | **Вне scope удаления** |
| `tvPairingUiPhase`, `amneziaIosPairingQrOverlayIsPresented` | Удалено |
| **Уровень 1** | **Закрыт** (2026-05-19): R8–R21, P1 |
| **P2–P6** | По необходимости (`canOpenTvQrPairingPage` → `v1/news` оставлен) |
| **Доп. чистка 2026-05-21** | `PageStart.qml` откат к `origin/dev` (churn не для pairing); `NSLog` torch / no-camera только под `#if DEBUG` в `iosPairingQrOverlayWindow.mm` (утверждённый план). |

---

*Правки в `client/` — по «Утверждаю план» / «Утверждаю действие». Обновления этого файла в `docs/plans/` — по задаче планирования.*
