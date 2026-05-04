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

Сервер слушает **`http://127.0.0.1:8080`** (в коде задано явно).

В логах должно появиться сообщение вида:

`plaintext mock listening on :8080  POST /v1/services  POST /v1/config`

## Эндпоинты

| Метод | Путь | Назначение |
|--------|------|------------|
| `POST` | `/v1/services` | Минимальный ответ со списком сервисов (в т.ч. `amnezia-free` / `awg`). |
| `POST` | `/v1/config` | Импорт конфига: лимит/CAPTCHA (`dchest/captcha`), проверка решения, мок-ответы. |

Других маршрутов нет.

## Связка с клиентом AmneziaVPN

1. Соберите клиент с флагом CMake **`AMNEZIA_LOCAL_GATEWAY=ON`** — тогда для `localhost` запросы к gateway уходят **plaintext JSON** без RSA/AES (см. `GatewayController`, `SecureAppSettingsRepository`).
2. В настройках приложения endpoint gateway должен указывать на **`http://localhost:8080/`** (или `http://127.0.0.1:8080/`). При включённом `AMNEZIA_LOCAL_GATEWAY` дефолтный URL в коде уже `http://localhost:8080/`.

После этого сценарии вроде **Amnezia Free → Continue** будут ходить в этот mock.

## Поведение CAPTCHA (для разработчика)

В `main.go` константа **`rateLimitExcessAfter`**: при `0` «лимит» срабатывает сразу и первый запрос к `/v1/config` для `amnezia-free` чаще возвращает ответ с CAPTCHA; большее значение имитирует N успешных запросов до CAPTCHA.

Опционально в теле `POST /v1/config` mock обрабатывает **`refresh_captcha": true`** (отдельная ветка в коде); кнопка «Обновить» в клиенте может повторно вызывать обычный импорт без этого поля — смотрите актуальную логику в `SubscriptionUiController`.

## Зависимости

- `github.com/dchest/captcha` — генерация и проверка картинки CAPTCHA.

После изменения зависимостей:

```bash
go mod tidy
```
