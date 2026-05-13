# Local gateway (plaintext mock)

Минимальный HTTP-сервер на Go, который имитирует ответы Amnezia API gateway **без шифрования**: те же JSON-тела, что клиент отправляет в зашифрованном виде на прод. Удобно для отладки UI (в том числе CAPTCHA) и сценария **Amnezia Free**.

## Требования

- [Go](https://go.dev/dl/) **1.21** или новее (см. `go.mod`).

## Запуск

Из каталога `tools/local_gateway`:

```bash
cd tools/local_gateway
go mod download
go run -buildvcs=false .
```

По умолчанию слушает **`0.0.0.0:8080`** (**`tcp4`** только — см. ниже). С Mac: `http://127.0.0.1:8080/`; с телефона в той же Wi‑Fi: `http://<LAN-IP-Mac>:8080/`.

**Флаги и переменные окружения** (полный чеклист: **[LAN_GATEWAY.md](./LAN_GATEWAY.md)**):

| Флаг / env | Назначение |
|------------|------------|
| `-listen` / `LOCAL_GATEWAY_LISTEN` | Адрес привязки, например `0.0.0.0:8080` (порт можно опустить — подставится `:8080`). |
| `-public-base` / `LOCAL_GATEWAY_PUBLIC_BASE` | Базовый URL **без** хвостового `/` для тела **`POST /v1/updater_endpoint`** (`url`). Нужен для **iOS по LAN** (иначе `127.0.0.1` указывает на сам телефон). |
| `-auto-public` (по умолчанию `true`) | Если `public-base` пуст, взять первый подходящий **не loopback** IPv4 и собрать `http://IP:порт`. |
| `-pairing-ttl`, `-long-poll` | TTL сессии QR и long-poll (по умолчанию **60s**, как в `tmp/updated_spec.yaml`; можно уменьшить для быстрых тестов). |
| `-rate-limit-excess-after` | Порог для мока Amnezia Free / CAPTCHA (по умолчанию **0**). |

Пример для iPhone + Mac:

```bash
LOCAL_GATEWAY_PUBLIC_BASE='http://192.168.1.10:8080' go run -buildvcs=false .
```

`net.Listen("tcp4", …)` оставлен, чтобы на macOS не ловить пустой ответ при `curl`/браузере на **LAN‑IPv4** (частая нестыковка IPv4/IPv6 у `ListenAndServe(":8080", …)`).

После `git pull` обязательно **остановите старый процесс** на порту (`Ctrl+C` или `kill <PID>`).

**Логи:** при старте печатается баннер с **`updater_endpoint` URL** и списком **`http://<ipv4>:порт/`** по интерфейсам. Каждый запрос: строки **`REQ start`** (remote addr, path, UA, `X-Client-Request-ID`, размер тела) и **`REQ end`** (HTTP status, длительность).

Проверка без клиента (mock должен быть запущен):

```bash
./verify.sh
# или
bash verify.sh http://127.0.0.1:8080
```

## Эндпоинты

| Метод | Путь | Назначение |
|--------|------|------------|
| `GET` | `/` | Короткий текст для проверки из браузера / телефона. |
| `GET` | `/VERSION` | Версия для цепочки обновлений (`UpdateController`: после `updater_endpoint`). Значение `0.0.1` — ниже клиента, «обновление не найдено». |
| `GET` | `/CHANGELOG` | Пустое тело, успех. |
| `GET` | `/RELEASE_DATE` | Пустое тело, успех. |
| `POST` | `/v1/account_info` | Экран API‑подписки (`getAccountInfo`). |
| `POST` | `/v1/services` | Каталог сервисов (`ServicesCatalogController`). |
| `POST` | `/v1/config` | Amnezia Free: CAPTCHA/лимит; иначе короткий мок‑ответ (полноценный premium `vpn://` здесь не строится). |
| `POST` | `/v1/news` | Лента новостей (`NewsController`), пустой `news`. |
| `POST` | `/v1/renewal_link` | Ссылка продления (`renewal_url`). |
| `POST` | `/v1/updater_endpoint` | `{"url":"…"}` — база из **`-public-base` / `LOCAL_GATEWAY_PUBLIC_BASE`** или **`-auto-public`**; затем клиент делает GET `/VERSION` на этом хосте. |
| `POST` | `/v1/revoke_config` | Успех, тело не разбирается при `NoError`. |
| `POST` | `/v1/revoke_native_config` | То же. |
| `POST` | `/api/v1/generate_qr` | Pairing: long-poll (дефолт **60s**); повтор с тем же `qr_uuid` до истечения TTL **продолжает** ожидание (resume). |
| `POST` | `/api/v1/scan_qr` | Pairing: завершение по `qr_uuid`; тело должно включать **`service_type`** и **`user_country_code`** (как в спецификации). |

**Не реализовано** (нужен осмысленный `vpn://` / IAP): `POST /v1/trial`, `POST /v1/subscriptions`, `POST /v1/native_config`, `POST /v1/proxy_config` (Telegram). При необходимости — отдельная доработка или прод gateway.

**Обновление premium** (`updateServiceFromGateway` → `POST /v1/config` с `amnezia-premium`) требует валидного поля `config` с `vpn://…` в ответе; текущий mock для premium не подменяет полный конфиг — избегайте «Reload API config» на полностью локальном стенде или расширяйте mock.

## Связка с клиентом AmneziaVPN

1. Включите **`AMNEZIA_QR_PAIRING_ALLOW=ON`** в CMake — тогда для **`127.0.0.1`**, **`localhost`**, **`::1`** запросы к gateway идут **plaintext JSON** без RSA/AES (`GatewayController`).
2. Для **iOS / другого хоста по LAN-IP** (`192.168.x.x` и т.п.) дополнительно включите **`AMNEZIA_LAN_PLAINTEXT_GATEWAY=ON`** (требует `AMNEZIA_QR_PAIRING_ALLOW`). Иначе клиент шифрует тело как к прод gateway, mock его не поймёт. Подробности: **[LAN_GATEWAY.md](./LAN_GATEWAY.md)**.
3. В настройках приложения укажите endpoint: **`http://127.0.0.1:8080/`** или **`http://<LAN-IP>:8080/`** (с завершающим `/`, как принято в репозитории).

Пошаговый план (включая следующие этапы вроде `/v1/account_info`): **`docs/local-gateway-mock.md`**.

После этого сценарии вроде **Amnezia Free → Continue** будут ходить в этот mock.

Для QR pairing (локальная разработка до готовности реального gateway):

1. TV-клиент вызывает `POST /api/v1/generate_qr` и держит long-poll (до **120s** в mock).
2. Phone-клиент вызывает `POST /api/v1/scan_qr` с тем же `qr_uuid`.
3. Mock возвращает TV-клиенту `200` c `config`, `service_info`, `supported_protocols`.

Поведение кодов:
- `generate_qr`: `200`, `400`, `408`, `500`
- `scan_qr`: `200`, `400`, `403`, `404`, `409`

Примечания:
- сессии хранятся in-memory (без Redis), TTL = **120s** (локально); на проде ожидайте **30s**;
- `auth_data.api_key == "invalid"` -> `403`;
- повторный `scan_qr` по завершенной сессии -> `409`.

## Быстрые `curl`-сценарии для QR pairing

## 1) Happy path (два терминала)

Терминал A (TV: long-poll ожидание):

```bash
curl -i -X POST "http://127.0.0.1:8080/api/v1/generate_qr" \
  -H "Content-Type: application/json" \
  -d '{
    "qr_uuid": "123e4567-e89b-12d3-a456-426614174000",
    "installation_uuid": "tv-installation-001",
    "app_version": "4.8.3.1",
    "os_version": "Android TV 14"
  }'
```

Терминал B (Phone: completion того же UUID):

```bash
curl -i -X POST "http://127.0.0.1:8080/api/v1/scan_qr" \
  -H "Content-Type: application/json" \
  -d '{
    "qr_uuid": "123e4567-e89b-12d3-a456-426614174000",
    "config": "vpn://AAAA_3icpVdtT-...",
    "service_info": {
      "ad_description": "Mock ad",
      "ad_endpoint": "https://example.com",
      "ad_header": "Try Premium",
      "is_ad_visible": false
    },
    "supported_protocols": ["awg", "vless"],
    "auth_data": {
      "api_key": "valid-local-key"
    },
    "installation_uuid": "phone-installation-001",
    "app_version": "4.8.3.1",
    "os_version": "Android 14"
  }'
```

Ожидаемо:
- в терминале B: `200 OK` + `{"message":"OK"}`
- в терминале A: `200 OK` + `config/service_info/supported_protocols`

## 2) Timeout path (`408`)

Вызовите только `generate_qr` и не отправляйте `scan_qr`:

```bash
curl -i -X POST "http://127.0.0.1:8080/api/v1/generate_qr" \
  -H "Content-Type: application/json" \
  -d '{
    "qr_uuid": "123e4567-e89b-12d3-a456-426614174111",
    "installation_uuid": "tv-installation-timeout",
    "app_version": "4.8.3.1",
    "os_version": "Android TV 14"
  }'
```

Через ~**120** секунд вернется `408 Request Timeout` (в mock).

## 3) Ошибка авторизации (`403`)

```bash
curl -i -X POST "http://127.0.0.1:8080/api/v1/scan_qr" \
  -H "Content-Type: application/json" \
  -d '{
    "qr_uuid": "123e4567-e89b-12d3-a456-426614174000",
    "config": "vpn://AAAA_3icpVdtT-...",
    "service_info": {"is_ad_visible": false},
    "supported_protocols": ["awg"],
    "auth_data": {"api_key": "invalid"},
    "installation_uuid": "phone-installation-001",
    "app_version": "4.8.3.1",
    "os_version": "Android 14"
  }'
```

Ожидаемо: `403 Forbidden`.

## Поведение CAPTCHA (для разработчика)

В `main.go` константа **`rateLimitExcessAfter`**: при `0` «лимит» срабатывает сразу и первый запрос к `/v1/config` для `amnezia-free` чаще возвращает ответ с CAPTCHA; большее значение имитирует N успешных запросов до CAPTCHA.

Опционально в теле `POST /v1/config` mock обрабатывает **`refresh_captcha": true`** (отдельная ветка в коде); кнопка «Обновить» в клиенте может повторно вызывать обычный импорт без этого поля — смотрите актуальную логику в `SubscriptionUiController`.

## Зависимости

- `github.com/dchest/captcha` — генерация и проверка картинки CAPTCHA.

После изменения зависимостей:

```bash
go mod tidy
```
