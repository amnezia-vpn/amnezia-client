# Интеграция протокола DNSTT в Amnezia VPN Client (Android)

Документ описывает архитектуру и сборку клиента **Amnezia VPN** с поддержкой протокола **DNSTT** — туннелирования трафика внутри DNS-запросов (DoH / DoT / UDP) к авторитативному серверу туннеля (`dnstt-server`).

---

## 1. Что такое DNSTT и его ограничения

DNSTT кодирует поток данных в имена DNS-запросов и получает ответы в TXT-записях. Поверх этого ненадёжного «канала» строится стек KCP → Noise NK → smux.

Ограничения, вытекающие из протокола, а не из реализации:

1. **Только TCP.** Туннель переносит поток байт, а не IP-пакеты. UDP-приложения (QUIC, VoIP, игры) не работают. DNS-запросы приложений транслируются клиентом в DNS-over-TCP.
2. **Очень маленький MTU.** Полезная нагрузка одного запроса ограничена длиной доменного имени (RFC 1035, 255 октетов):

   ```
   capacity = (255 - 1 - Σ(len(label) + 1)) * 63/64 * 5/8
   MTU      = capacity - 8 (clientid) - 1 (padding len) - 3 (padding) - 1 (data len)
   ```

   Требуется `MTU ≥ 80`, иначе домен непригоден. Типичная пропускная способность — 100–500 Кбит/с.
3. **Нужен SOCKS5 на стороне сервера.** `dnstt-server` пересылает каждый поток на один фиксированный адрес, поэтому произвольные назначения достижимы, только если этот адрес — SOCKS5-прокси.
4. **Резолвер должен быть достижим вне туннеля.** Сокеты клиента исключаются из VPN через `VpnService.protect()`. Если резолвер задан именем, а не IP, нужен bootstrap-IP.

### Путь данных

```
[Android-приложения]
        │ IP-пакеты
        ▼
  [VpnService TUN fd]
        │
        ▼
[libdnstt.so: gVisor netstack (tun2socks/core)]
   ├── TCP     ──▶ smux-поток ──▶ SOCKS5 CONNECT
   └── UDP:53  ──▶ smux-поток ──▶ SOCKS5 CONNECT ──▶ DNS-over-TCP
                        │
                        ▼
        [smux ← Noise NK ← KCP ← DNSPacketConn]
                        │ DoH POST / DoT / UDP 53
                        ▼
              [Публичный резолвер]
                        │
                        ▼
                [dnstt-server (VPS)]
                        │
                        ▼
                [SOCKS5 → Internet]
```

Прочий UDP отбрасывается: у туннеля нет способа его перенести, и выпускать его мимо туннеля недопустимо.

---

## 2. Состав по слоям

### 2.1. Go-ядро (`client/3rd/dnstt/`)

Протокольная часть **вендорена из upstream** `https://www.bamsoftware.com/git/dnstt.git` (David Fifield, лицензия CC0 1.0 — см. `COPYING.upstream`). Переписан только путь модуля; логика формата не менялась, чтобы сохранить совместимость с `dnstt-server`.

| Путь | Происхождение | Назначение |
|------|---------------|------------|
| `dns/` | upstream | Кодирование/разбор DNS-сообщений и имён |
| `noise/` | upstream | Noise NK: рукопожатие и шифрование сессии |
| `turbotunnel/` | upstream | Очереди пакетов, client ID, `DummyAddr` |
| `dnsttclient/dns.go`, `http.go`, `tls.go`, `weightedlist.go` | upstream | `DNSPacketConn`, DoH/DoT транспорты, взвешенные списки |
| `dnsttclient/utls.go` | upstream + правка | uTLS-фингерпринты; добавлен проброс `DialFunc` для `protect()` |
| `dnsttclient/tunnel.go` | наше | Сборка стека, выбор резолвера с failover, старт/стоп |
| `dnsttclient/netstack.go` | наше | gVisor-стек на TUN fd, обработчики TCP и UDP/53 |
| `dnsttclient/socks.go` | наше | SOCKS5-клиент поверх smux-потока |
| `jni/` | наше | CGO JNI (`libdnstt.so`): экспорт функций, колбэк `protect()`, проброс логов |

Ключевые зависимости: `xtaci/kcp-go`, `xtaci/smux`, `flynn/noise`, `refraction-networking/utls`, `xjasonlyu/tun2socks` (пакет `core`), `gvisor.dev/gvisor`.

> Версия gvisor **запинена** в `go.mod` на ту, что использует tun2socks. На tip в пакете `pkg/tcpip/stack` лежит тестовый файл с другим `package`, из-за чего сборка падает.

### 2.2. Android (`client/android/`)

* **`dnstt/src/main/kotlin/DnsttNative.kt`** — JNI-мост:
  * `startTunnel(tunFd, tunMtu, domain, resolvers, bootstrapIp, pubKey): String?` — возвращает `null` при успехе, иначе текст ошибки;
  * `stopTunnel(): String?`, `calculateMtu(domain): Int`;
  * `protectSocket(fd)` и `nativeLog(msg)` — вызываются **из** native-кода.
* **`dnstt/src/main/kotlin/Dnstt.kt`** — реализация `Protocol`. Передаёт fd через `detachFd()` (владение переходит в Go), ставит `DnsttNative.protector` на время сессии, валидирует домен и ключ.
* **`DnsttService.kt`**, `VpnProto.kt`, `AndroidManifest.xml` — сервис в отдельном процессе `:amneziaDnsttService`.

MTU TUN-интерфейса — обычные 1500. Payload-MTU туннеля от него не зависит и вычисляется внутри `libdnstt`.

### 2.3. C++ ядро (`client/core/`)

* **`models/protocols/dnsttProtocolConfig.{h,cpp}`** — поля конфига, расчёт MTU, валидация. Хранится по конвенции проекта: `{"dnstt": {"last_config": "<json>", "isThirdPartyConfig": true}}`; на Android уходит `dnstt_config_data` со snake_case-ключами (`domain`, `resolvers`, `bootstrap_ip`, `public_key`).
* **`models/protocolConfig.cpp`** — ветки `DnsttProtocolConfig` в `type()`, `fromJson()`, `getClientConfigJson()`, `hasClientConfig()`, `isThirdPartyConfig()`.
* **`controllers/selfhosted/importController.cpp`** — разбор ссылки:
  `dnstt://<publicKey>@<domain>/?resolvers=<url-encoded>&bootstrap=<ip>`
* **`utils/containers/containerUtils.cpp`** — контейнер `amnezia-dnstt` доступен только на Android; серверной части (docker-образа, install-скриптов) у него нет.

### 2.4. UI (`client/ui/`)

* **`models/protocols/dnsttConfigModel.{h,cpp}`** — модель формы, зарегистрирована как QML-свойство `DnsttConfigModel` в `coreController`.
* **`qml/Pages2/PageSetupWizardDnsttSettings.qml`** — страница мастера. Вся валидация и расчёт MTU идут через модель (C++), в QML не дублируются.
* **`qml/Pages2/PageSetupWizardConfigSource.qml`** — карточка «DNSTT (DNS Tunnel)», видима только на Android.

---

## 3. Сборка

### 3.1. Окружение

* **Qt**: 6.10.0 (`gcc_64` + `android_arm64_v8a`), модули `qt5compat`, `qtshadertools`, `qtremoteobjects`
* **Android SDK**: `platforms;android-28`+, `build-tools;35.0.0`
* **Android NDK**: `26.3.11579264`
* **Go**: 1.26+ (нужен для `libdnstt.so`)
* **Java**: OpenJDK 17

### 3.2. Как собирается libdnstt.so

Отдельного шага больше нет: `client/cmake/android.cmake` собирает Go-модуль NDK-тулчейном как часть CMake-сборки и кладёт результат в `client/android/libs/<ABI>/libdnstt.so` (каталог в `.gitignore`). ABI → GOARCH определяется автоматически. Если Go не найден, сборка не падает, но выводит предупреждение и DNSTT в такой сборке работать не будет.

Обычной команды достаточно:

```bash
./deploy/build.sh -t android --abi arm64-v8a
```

### 3.3. Полный цикл до подписанного APK

```bash
wsl -u root bash /mnt/d/projects/amnezia-client/deploy/build_android_wsl.sh
```

Скрипт выполняет: CMake-сборку (включая `libdnstt.so`) → `patch_libs_xml.py` → `gradlew assembleRelease` → `zipalign` → `apksigner`. Результат: `deploy/build/AmneziaVPN-dnstt.apk`.

`deploy/patch_libs_xml.py` вносит все `.so` QML-плагинов в `res/values/libs.xml`: с Android 10 bionic блокирует `dlopen()` по пути для библиотек, не загруженных через `System.loadLibrary`. Скрипт принимает каталог сборки аргументом и обрабатывает все ABI.

---

## 4. Решённые проблемы сборки

| № | Симптом | Причина | Решение |
|---|---------|---------|---------|
| 1 | APK не устанавливался | Нет подписи v2/v3 и выравнивания | `zipalign -p 4` + `apksigner sign` в пайплайне |
| 2 | `UnsatisfiedLinkError` через 1 с | `gomobile bind` давал `libgojni.so`, конфликтующий с таким же из `libxray.aar` | Go-ядро переведено на прямой CGO JNI (`libdnstt.so`); `.aar` и `pickFirst` для `libgojni.so` удалены |
| 3 | SIGSEGV в `qt_qFindChild_helper` | В `FocusController` слот `objectCreated` не проверял `nullptr` | Добавлена проверка `if (!object) return;` |
| 4 | `dlopen failed: library ... not found` | Изоляция bionic namespace на Android 10+ | `deploy/patch_libs_xml.py` |
| 5 | `libQt6ShaderTools_...so not found` | `Qt5Compat.GraphicalEffects` требует Qt6ShaderTools, который не деплоится, пока не слинкован | `Qt6::ShaderTools` линкуется **только для Android** (на desktop модуль не нужен) |
| 6 | `duplicate symbol: JNI_OnLoad` | cgo компилирует преамбулу в несколько объектов | `JNI_OnLoad` вынесен в отдельный `jni/jni_helper.c` |
| 7 | `found packages stack and bridge` | В gvisor на tip тестовый файл с чужим `package` | Версия gvisor запинена под tun2socks |

---

## 5. Что проверено, а что нет

**Проверено:** сборка `libdnstt.so` (arm64, экспорт `JNI_OnLoad` и трёх JNI-функций), сборка C++/QML, сборка и подпись APK.

**Не проверено:** работа туннеля против живого `dnstt-server` — стенда с сервером и делегированной зоной не было. Протокольная часть взята из upstream без изменений формата, но интеграционные слои (netstack, SOCKS5, DNS-over-TCP, `protect()`) на реальном соединении не прогонялись.

Для проверки на стенде нужен `dnstt-server`, делегированная NS-зона и SOCKS5-прокси за ним; логи `libdnstt` видны в logcat по тегу `libdnstt`.
