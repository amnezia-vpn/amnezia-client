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

В логах должно появиться сообщение вида:

`plaintext mock listening on 0.0.0.0:8080  GET /  POST /v1/services  POST /v1/config  POST /api/v1/generate_qr  POST /api/v1/scan_qr`

## Эндпоинты

| Метод | Путь | Назначение |
|--------|------|------------|
| `GET` | `/` | Короткий текст для проверки из браузера / телефона. |
| `POST` | `/v1/services` | Минимальный ответ со списком сервисов (в т.ч. `amnezia-free` / `awg`). |
| `POST` | `/v1/config` | Импорт конфига: лимит/CAPTCHA (`dchest/captcha`), проверка решения, мок-ответы. |
| `POST` | `/api/v1/generate_qr` | Регистрация pairing-сессии по `qr_uuid` + long-poll (**120s** в этом mock; **30s** на production gateway). |
| `POST` | `/api/v1/scan_qr` | Завершение pairing-сессии: передача `config` + `service_info` + `supported_protocols` по `qr_uuid`. |

Других маршрутов нет (кроме `GET /`).

## Связка с клиентом AmneziaVPN

1. Соберите клиент с флагом CMake **`AMNEZIA_LOCAL_GATEWAY=ON`** — тогда для `localhost` запросы к gateway уходят **plaintext JSON** без RSA/AES (см. `GatewayController`, `SecureAppSettingsRepository`).
2. В настройках приложения endpoint gateway должен указывать на **`http://localhost:8080/`** (или `http://127.0.0.1:8080/`). При включённом `AMNEZIA_LOCAL_GATEWAY` дефолтный URL в коде уже `http://localhost:8080/`.

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
