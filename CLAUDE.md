# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**FBLink VPN** — форк AmneziaVPN. Состоит из трёх компонентов:
1. **Qt6 клиент** (`client/`) — C++/QML, кроссплатформенный desktop/mobile GUI
2. **Go backend API** (`vpn-backend/`) — Gin + SQLite, управление пользователями/подписками/конфигами
3. **Системный сервис** (`service/`) — привилегированный C++ daemon (WireGuard, firewall, routing)

## Сборка

### Windows (клиент)
```bat
deploy\build_windows.bat
```
Требует: Qt6, WiX Toolset, Qt Installer Framework, MinGW. Выводит `FBLinkVPN_x64.exe` и `.msi`.

### Ручная сборка через CMake (для разработки)
```bash
# В Qt Creator: открыть CMakeLists.txt (корневой), выбрать Kit, нажать Build
# Или вручную:
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --parallel
```

При смене папки проекта — удалить `build/*/CMakeCache.txt` (CMake кэширует абсолютные пути).

### Go backend
```bash
cd vpn-backend
go run ./cmd/server          # запуск
go build ./cmd/server        # сборка
docker-compose up -d         # в Docker
```

### Linux/macOS/Android/iOS
```bash
deploy/build_linux.sh
deploy/build_macos.sh
deploy/build_android.sh
deploy/build_ios.sh
```

## Архитектура

### Клиент (Qt6 C++/QML)
```
client/
├── core/controllers/     # Бизнес-логика: coreController, gatewayController, serverController
├── protocols/            # VPN протоколы: WireGuard, AWG, OpenVPN, XRay, IKEv2, Shadowsocks
├── configurators/        # Генераторы конфигов для каждого протокола
├── ui/
│   ├── controllers/      # QML-C++ мост: pageController, connectionController, etc.
│   ├── models/           # Qt модели для QML ListView
│   └── qml/Pages2/       # QML страницы UI
├── daemon/               # Интерфейс к системному сервису (IPC клиент)
└── platforms/            # Платформо-специфичный код (windows/, macos/, linux/, android/, ios/)
```

**Поток данных клиента**: QML ↔ Controller (C++) ↔ Core Logic ↔ IPC ↔ Service (daemon)

Контроллеры регистрируются в QML через `qmlRegisterSingletonType` в `coreController.cpp`. Страницы QML находятся в `client/ui/qml/Pages2/`.

### FBLink-специфичные контроллеры
- `client/ui/controllers/api/fblink_controller.cpp` — авторизация/подписка через Go backend
- QML страницы: `PageFBLinkLogin.qml`, `PageFBLinkRegister.qml`, `PageFBLinkSubscription.qml`

### Go Backend (`vpn-backend/`)
```
internal/
├── handlers/    # auth.go, user.go, vpn.go, payment.go, admin.go, renewal.go, ssh_peer.go
├── models/      # User, Server, Subscription, Payment (GORM модели)
├── middleware/  # JWT auth, admin role check
├── config/      # Конфиг из .env файла
├── backup/      # Плановый бэкап БД на email администраторов
└── router/      # Gin роутер — все эндпоинты
```

**API структура**:
- `/api/v1/auth/*` — публичные (register, login, refresh)
- `/api/v1/me/*` — требуют JWT (профиль, подписка, WG конфиг)
- `/api/v1/payments/*` — создание платежа + webhook YooKassa
- `/api/v1/admin/*` — только для role=admin
- `/admin/` — веб-панель администратора (static HTML)

### Системный Сервис (`service/`)
Запускается с правами администратора. Управляет WireGuard интерфейсом, iptables/firewall, маршрутизацией. Общается с клиентом через Qt IPC (`.rep` файлы в `ipc/`).

## Ключевые технические детали

### Переменные окружения клиента
Задаются в `client/CMakeLists.txt` через `target_compile_definitions`:
- `AGW_URI_PROD` / `AGW_URI_DEV` — URL Go backend
- `YOOKASSA_SHOP_ID_*` — YooKassa интеграция
- `S3_*` — S3 хранилище

### Backend .env
```
SMTP_HOST, SMTP_PORT, SMTP_USER, SMTP_PASSWORD, SMTP_FROM — для отправки бэкапов
JWT_SECRET, YOOKASSA_SHOP_ID, YOOKASSA_SECRET_KEY
DB_PATH — путь к SQLite файлу
```

### Платежи (YooKassa)
Webhook: `POST /api/v1/payments/webhook`. Логика в `handlers/payment.go`:
- Trial план всегда начинается с текущей даты (не стакается поверх активной подписки)
- Остальные планы стакаются поверх текущего `ExpiresAt` если он в будущем

### Даты подписки в QML
ISO строки из Go содержат наносекунды (`2026-03-19T...778881751Z`) — JS `new Date()` их не парсит. Всегда использовать `.slice(0, 10)`:
```qml
Qt.formatDate(new Date(FBLinkController.subscriptionEndDate.slice(0, 10)), "d MMMM yyyy")
```

### SMTP (бэкап)
В `smtp.SendMail` envelope sender должен быть plain email (`cfg.SMTPUser`), не display name. Display name используется только в MIME заголовке `From:`.

### Переводы
Файлы в `client/translations/amneziavpn_*.ts`. После изменения строк в QML/C++ — запустить `lupdate` через Qt Creator (`Tools → External → Qt Linguist → Update Translations`).

## Именование

Проект — форк AmneziaVPN. В коде сохранены некоторые внутренние названия Amnezia (классы, namespace), но UI-строки заменены на FBLink VPN. Не переименовывать внутренние C++ классы и Qt namespace — только UI-видимые строки.
