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
- [x] **Фаза 4** — `util::ThreadPool` (drain в деструкторе), `postAsync`(коллбэк на потоке пула)/
  `postFuture`(`std::future`) поверх `executePost`, `CancellationToken` (проверки между шагами
  failover + прерывание трансфера через progress-коллбэк curl → `ErrorCode::Cancelled`). Кеш прокси
  под мьютексом, пул — последний член Impl (рушится первым, дожидаясь задач). TSan чисто; ASan+UBSan
  10/10.
- [x] **Фаза 5** — C-ABI (`include/agw/c_abi.h` + `src/c_abi.cpp`, `agw_*`: создание/уничтожение,
  sync/async `post`, токен отмены, освобождение результата; на границе только C-типы). Сборка:
  object-библиотека → static `agw` + shared `agw_capi` (экспортирует **только** `agw_*`, остальное
  скрыто). C-smoke (чистый C) и Dart-smoke (`dart:ffi`) проходят. `conan create` (shared-deps)
  зелёный — пакет с `libagw.a` + `libagw_capi.dylib` + заголовками. Режим `vendored` (статические
  зависимости) задан в conanfile (`-o deps_mode=vendored`).
- [~] **Фаза 6** — интеграция в Qt-клиент. Готово: `GatewayController` переписан тонким адаптером
  над `agw::GatewayClient` (сигнатуры один в один, байт-паритет payload, персистентный клиент на
  окружение, `onBeforeRequest` = iOS inet + desktop kill-switch, async через `QPromise`+маршалинг);
  проводка сборки (корневой `conanfile` requires `agw-sdk/0.1.0`, `client/cmake/3rdparty.cmake`
  линкует `agw::agw`). Осталось (вне этого окружения): Qt-сборка под все платформы, перевод
  синхронных вызовов (`subscription`/`servicesCatalog` `executeRequest`) на рабочий поток, регрессия
  против dev/prod. См. `docs/plans/gateway-sdk/agw-sdk-tier1-phase6-integration.md`.

## Раскладка

```
include/agw/   публичные заголовки (types, config, client, http, cancellation, c_abi)
src/crypto/    AES, RSA, SHA-512, RNG
src/util/      base64, uuid, json (Qt-Indented), url, thread_pool
src/protocol/  имена полей API, request_builder, response, error_mapping
src/failover/  bypass_policy, proxy_list, proxy_picker
src/http/      curl_client (+ fallback)
src/c_abi.cpp  C-ABI обёртка
tests/         unit + golden + integration (+ вендоренный nlohmann для офлайн-сборки)
examples/      c_smoke (чистый C), dart_smoke (dart:ffi)
```

## C-ABI и потребление из Dart/C

Публичный C-заголовок — `include/agw/c_abi.h`. Shared-библиотека `libagw_capi.*` экспортирует только
`agw_*`. Примеры:

```sh
# чистый C
cc -std=c11 -Iinclude examples/c_smoke/smoke.c -Lbuild-local -lagw_capi -o /tmp/agw_smoke
DYLD_LIBRARY_PATH=build-local /tmp/agw_smoke         # → код 1105, OK

# Dart (dart:ffi)
cd examples/dart_smoke && dart pub get && dart run   # → код 1105, OK
```

## Локальная сборка и тесты (без Conan)

Нужны CMake ≥ 3.21 и OpenSSL 3. nlohmann/json берётся из вендоренного single-header
(`tests/third_party`), если Conan-пакет не найден.

```sh
cmake -S . -B build-local -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
cmake --build build-local -j
ctest --test-dir build-local --output-on-failure
```

Санитайзеры (macOS): TSan — на конкурентных тестах; ASan+UBSan — `detect_leaks=0` (LSan на Darwin
не поддержан):

```sh
cmake -S . -B build-asan -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3) \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g -O1" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan -j
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build-asan --output-on-failure
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
