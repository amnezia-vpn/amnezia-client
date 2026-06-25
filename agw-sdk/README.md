# agw-sdk

Qt-free C++20 транспорт к API-шлюзу Amnezia (вынос `GatewayController`). Узкая поверхность —
`post` (sync/async) поверх крипты, выбора эндпоинта и обхода блокировок. Протокол воспроизводится
байт-в-байт.

План и решения: [../docs/plans/gateway-sdk/](../docs/plans/gateway-sdk/) — начни с
`agw-sdk-tier1-impl-plan.md` и `README.md` (таблица решений).

## Статус

Тир 1, в работе по фазам:

- [x] **Фаза 1** — каркас + крипта на OpenSSL EVP (AES-256-CBC, RSA-PKCS1 v1.5, SHA-512), base64
  (std + url), UUID v4, Qt-Indented JSON-сериализатор, golden-тесты крипты.
- [x] **Фаза 2** — `IHttpClient`(libcurl) + `Config`/`GatewayClient`/`executePost` + sync `post`;
  `request_builder`/`response`/`error_mapping`; интеграционный тест через in-process mock-шлюз
  (полный round-trip: SDK шифрует → «сервер» расшифровывает → шифрует ответ → SDK расшифровывает).
- [x] **Фаза 3** — failover: `bypass_policy` (`shouldBypassProxy` дословно), `proxy_list`
  (S3-пути + prod-расшифровка через `SHA-512(pubkey)`), `proxy_picker` (health-check `lmbd-health`),
  встройка в `executePost` с кешем рабочего прокси на инстансе (под мьютексом). Интеграционный тест:
  прямой ответ подозрителен → S3 → health → прокси → успех; повторный запрос идёт сразу на кеш.
- [ ] Фаза 4 — async/`CancellationToken`, пул потоков.
- [ ] Фаза 5 — C-ABI + режимы сборки + Dart-smoke.
- [ ] Фаза 6 — интеграция в Qt-клиент через адаптер.

## Раскладка

```
include/agw/   публичные заголовки (types.h; config/client/http — Фаза 2+)
src/crypto/    AES, RSA, SHA-512, RNG
src/util/      base64, uuid, json (Qt-Indented), log/thread_pool — позже
src/protocol/  имена полей API; request_builder/response/error_mapping — Фаза 2
src/failover/  Фаза 3
tests/         unit + golden (+ вендоренный nlohmann для офлайн-сборки)
```

## Локальная сборка и тесты (без Conan)

Нужны CMake ≥ 3.21 и OpenSSL 3. nlohmann/json берётся из вендоренного single-header
(`tests/third_party`), если Conan-пакет не найден.

```sh
cmake -S . -B build-local -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
cmake --build build-local -j
ctest --test-dir build-local --output-on-failure
```

## Сборка через Conan (как в проекте)

```sh
conan create . -o build_tests=True
```

Зависимости: `openssl/3.6.2`, `nlohmann_json/3.11.3` (как в корневом `conanfile.py`); `libcurl` —
с Фазы 2.

## Заметки по паритету

Крипта сверена с `client/3rd/QSimpleCrypto` и `gatewayController.cpp`. Ключевое:

- AES-256-CBC, ключ 32 байта, IV генерится 32 — CBC берёт первые 16; salt (8 байт) в локальном AES
  не участвует, уходит только в `key_payload`.
- RSA PKCS#1 v1.5 — паддинг рандомный, поэтому `key_payload` **не** воспроизводим байт-в-байт;
  golden проверяет его round-trip, а `api_payload` (AES) — точные байты.
- JSON собирается в формате `QJsonDocument::toJson(Indented)`: отступ 4 пробела, завершающий `\n`,
  **отсортированные ключи** (это даёт `aes_iv` раньше `aes_key`).
