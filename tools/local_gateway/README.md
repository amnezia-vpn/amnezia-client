# Local gateway (plaintext mock)

Минимальный HTTP-сервер на Go, который имитирует ответы Amnezia API gateway **без шифрования**: те же JSON-тела, что клиент отправляет в зашифрованном виде на прод. Удобно для отладки UI (в том числе CAPTCHA) и сценария **Amnezia Free**.

## Требования

- [Go](https://go.dev/dl/) **1.21** или новее (см. `go.mod`).

## Запуск

Из каталога `tools/local_gateway`:

```bash
cd tools/local_gateway
go mod download
go run .
```

Сервер слушает **`0.0.0.0:8080`** (все IPv4‑интерфейсы): с этого Mac — `http://127.0.0.1:8080/`, с телефона в той же LAN — `http://<IP-это-машины>:8080/`.

Сервер поднимается через **`net.Listen("tcp4", "0.0.0.0:8080")`**, чтобы на macOS не ловить пустой ответ при `curl`/браузере на **LAN‑IP** (частая нестыковка IPv4/IPv6 у `ListenAndServe(":8080", …)`).

После `git pull` обязательно **остановите старый процесс** на 8080 (`Ctrl+C` в терминале или `kill <PID>`), иначе будет крутиться бинарник без правок.

В логах при старте: `plaintext mock on tcp4 0.0.0.0:8080 — see ... README.md for paths`. Каждый запрос дополнительно пишется как `REQ <METHOD> <path>`.

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
| `POST` | `/v1/updater_endpoint` | `{"url":"http://127.0.0.1:8080"}` → затем GET `/VERSION` на этом хосте. |
| `POST` | `/v1/revoke_config` | Успех, тело не разбирается при `NoError`. |
| `POST` | `/v1/revoke_native_config` | То же. |
| `POST` | `/api/v1/generate_qr` | Pairing: long-poll (**120s** mock). |
| `POST` | `/api/v1/scan_qr` | Pairing: завершение по `qr_uuid`. |

**Не реализовано** (нужен осмысленный `vpn://` / IAP): `POST /v1/trial`, `POST /v1/subscriptions`, `POST /v1/native_config`, `POST /v1/proxy_config` (Telegram). При необходимости — отдельная доработка или прод gateway.

**Обновление premium** (`updateServiceFromGateway` → `POST /v1/config` с `amnezia-premium`) требует валидного поля `config` с `vpn://…` в ответе; текущий mock для premium не подменяет полный конфиг — избегайте «Reload API config» на полностью локальном стенде или расширяйте mock.

## Связка с клиентом AmneziaVPN

1. Соберите клиент с определением **`AMNEZIA_LOCAL_GATEWAY`** (см. `client/CMakeLists.txt`, `target_compile_definitions`) — тогда для **`127.0.0.1`** и **`localhost`** запросы к gateway уходят **plaintext JSON** без RSA/AES (см. `GatewayController`, `SecureAppSettingsRepository`).
2. В настройках приложения endpoint gateway: **`http://127.0.0.1:8080/`** (дефолт при `AMNEZIA_LOCAL_GATEWAY` в коде). Допустим и `http://localhost:8080/` — тоже plaintext.

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
