# Анализ и улучшение архитектуры моделей

## Анализ текущего кода

### Текущая структура

1. **ProtocolConfig** = `std::variant<AwgProtocolConfig, WireGuardProtocolConfig, ...>` (11 типов)
   - Все операции в `ProtocolConfigUtils` (20+ функций)
   - Используется в 8+ файлах через утилиты

2. **ContainerConfig** = структура с `ProtocolConfig protocolConfig`
   - Имеет методы: `getProtocolType()`, `toJson()`, `fromJson()`
   - Но все еще использует `ProtocolConfigUtils::toJson()` внутри
   - Имеет helper-методы типа `getAwgProtocolConfig()` через `std::get_if`

3. **ServerConfig** = `std::variant<SelfHostedServerConfig, NativeServerConfig, ApiV1ServerConfig, ApiV2ServerConfig>`
   - Все операции в `ServerConfigUtils` (15+ функций)
   - Используется в 15+ файлах через утилиты

### Проблемы, выявленные в коде

#### 1. Размазанная логика доступа к полям

**Пример из `serversModel.cpp`:**
```cpp
QString name = ServerConfigUtils::description(server);
if (name.isEmpty()) {
    return ServerConfigUtils::hostName(server);
}
QString primaryDns = ServerConfigUtils::dns1(server);
DockerContainer container = ServerConfigUtils::defaultContainer(server);
```

**Пример из `installController.cpp`:**
```cpp
QString port = ProtocolConfigUtils::portWithDefault(config.protocolConfig, protocol);
QString transportProto = ProtocolConfigUtils::transportProtoWithDefault(config.protocolConfig, protocol);
bool hasClient = ProtocolConfigUtils::hasClientConfig(containerConfig.protocolConfig);
```

Все это простой доступ к полям, но через утилиты.

#### 2. Множественные проверки типа

**Пример из `serversModel.cpp`:**
```cpp
if (ServerConfigUtils::isApiV1Config(server)) {
    return ServerConfigUtils::asApiV1(server).name;
} else if (ServerConfigUtils::isApiV2Config(server)) {
    return ServerConfigUtils::asApiV2(server).name;
}
if (ServerConfigUtils::isApiV2Config(server)) {
    return ServerConfigUtils::asApiV2(server).apiConfig.availableCountries;
}
```

Проверки типа разбросаны по коду.

#### 3. Дублирование логики в утилитах

**Пример из `protocolConfig.cpp`:**
```cpp
QString ProtocolConfigUtils::port(const ProtocolConfig& config) {
    return std::visit([](auto&& arg) -> QString {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) {
            return arg.serverConfig.port;
        } else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) {
            return arg.serverConfig.port;
        } // ... еще 9 типов
    }, config);
}
```

Та же логика повторяется для `transportProto`, `hasClientConfig` и т.д.

#### 4. ContainerConfig уже частично инкапсулирован

**Текущий код:**
```cpp
struct ContainerConfig {
    DockerContainer container;
    ProtocolConfig protocolConfig;
    
    Proto getProtocolType() const;  // ✅ Уже есть
    QJsonObject toJson() const;     // ✅ Уже есть, но использует ProtocolConfigUtils
    static ContainerConfig fromJson(const QJsonObject& json);
    
    // Helper методы через std::get_if
    AwgProtocolConfig* getAwgProtocolConfig();
};
```

Частичная инкапсуляция уже есть, но неполная.

---

## Архитектурный анализ

### Это поведенческая иерархия или структурная модель данных?

**Анализ операций в утилитах:**

1. **ProtocolConfigUtils:**
   - `port()` - доступ к полю `serverConfig.port` или `port`
   - `transportProto()` - доступ к полю `serverConfig.transportProto`
   - `hasClientConfig()` - проверка `clientConfig.has_value()`
   - `getClientConfigJson()` - вызов `clientConfig->toJson()`
   - `nativeConfig()` - доступ к полю `clientConfig->nativeConfig`
   - `isThirdPartyConfig()` - доступ к полю `serverConfig.isThirdPartyConfig`

2. **ServerConfigUtils:**
   - `description()` - доступ к полю `description`
   - `hostName()` - доступ к полю `hostName`
   - `containers()` - доступ к полю `containers`
   - `defaultContainer()` - доступ к полю `defaultContainer`
   - `dns1()`, `dns2()` - доступ к полям

**Вывод:** Это **структурные данные (DTO)**, а не поведенческая иерархия. Все операции — это доступ к полям, сериализация и форматирование.

**Вариант с полиморфизмом (`virtual` + `unique_ptr`) не оправдан**, так как:
- Нет разной бизнес-логики для разных типов
- Теряем value semantics
- Добавляем heap-аллокации без выигрыша

---

## Рекомендуемое решение

### Принцип: Инкапсулировать операции в методы структур

Оставить `variant` для данных, но сделать все операции **методами структур**, а не утилитами.

### Конкретные изменения

#### 1. ProtocolConfig → обернуть в структуру с методами

**Текущее:**
```cpp
using ProtocolConfig = std::variant<AwgProtocolConfig, WireGuardProtocolConfig, ...>;

namespace ProtocolConfigUtils {
    QString port(const ProtocolConfig& config);
    QString transportProto(const ProtocolConfig& config);
    bool hasClientConfig(const ProtocolConfig& config);
    // ... 20+ функций
}
```

**Предлагаемое:**
```cpp
struct ProtocolConfig {
    using Variant = std::variant<
        AwgProtocolConfig,
        WireGuardProtocolConfig,
        OpenVpnProtocolConfig,
        XrayProtocolConfig,  // also used for SSXray
        SftpProtocolConfig,
        Socks5ProxyProtocolConfig,
        Ikev2ProtocolConfig,
        TorProtocolConfig,
        DnsProtocolConfig
    >;
    
    Variant data;
    
    // Все операции - методы структуры
    Proto type() const;
    QString port() const;
    QString transportProto() const;
    QString portWithDefault(Proto protocol) const;
    QString transportProtoWithDefault(Proto protocol) const;
    bool hasClientConfig() const;
    QString clientId() const;
    QJsonObject getClientConfigJson() const;
    void setClientConfigJson(const QJsonObject& json);
    void clearClientConfig();
    QString nativeConfig() const;
    bool isThirdPartyConfig() const;
    QJsonObject toJson() const;
    
    // Factory
    static ProtocolConfig fromJson(const QJsonObject& json, Proto type);
    
    // Helper для доступа к конкретному типу (опционально)
    template<typename T>
    T* as() { return std::get_if<T>(&data); }
    
    template<typename T>
    const T* as() const { return std::get_if<T>(&data); }
};
```

**Реализация методов:**
```cpp
Proto ProtocolConfig::type() const {
    return std::visit([](const auto& v) -> Proto {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig>) return Proto::Awg;
        else if constexpr (std::is_same_v<T, WireGuardProtocolConfig>) return Proto::WireGuard;
        // ... остальные типы
    }, data);
}

QString ProtocolConfig::port() const {
    return std::visit([](const auto& v) -> QString {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, AwgProtocolConfig> || 
                      std::is_same_v<T, WireGuardProtocolConfig> ||
                      std::is_same_v<T, OpenVpnProtocolConfig> ||
                      std::is_same_v<T, XrayProtocolConfig> ||
                      std::is_same_v<T, Ikev2ProtocolConfig>) {
            return v.serverConfig.port;
        } else if constexpr (std::is_same_v<T, SftpProtocolConfig> ||
                             std::is_same_v<T, Socks5ProxyProtocolConfig>) {
            return v.port;
        }
        return QString();
    }, data);
}
```

#### 2. ContainerConfig → использовать методы ProtocolConfig

**Текущее:**
```cpp
QJsonObject ContainerConfig::toJson() const {
    QJsonObject obj;
    obj[configKey::container] = ContainerUtils::containerToString(container);
    Proto protoType = getProtocolType();
    QString protoName = ProtocolUtils::protoToString(protoType);
    QJsonObject protoJson = ProtocolConfigUtils::toJson(protocolConfig, protoType);  // ❌ Утилита
    obj[protoName] = protoJson;
    return obj;
}
```

**Предлагаемое:**
```cpp
QJsonObject ContainerConfig::toJson() const {
    QJsonObject obj;
    obj[configKey::container] = ContainerUtils::containerToString(container);
    Proto protoType = protocol.type();  // ✅ Метод
    QString protoName = ProtocolUtils::protoToString(protoType);
    obj[protoName] = protocol.toJson();  // ✅ Метод
    return obj;
}

Proto ContainerConfig::getProtocolType() const {
    return protocol.type();  // ✅ Вместо ContainerUtils::defaultProtocol(container)
}
```

#### 3. ServerConfig → обернуть в структуру с методами

**Текущее:**
```cpp
using ServerConfig = std::variant<SelfHostedServerConfig, NativeServerConfig, ...>;

namespace ServerConfigUtils {
    QString description(const ServerConfig& config);
    QString hostName(const ServerConfig& config);
    // ... 15+ функций
}
```

**Предлагаемое:**
```cpp
struct ServerConfig {
    using Variant = std::variant<
        SelfHostedServerConfig,
        NativeServerConfig,
        ApiV1ServerConfig,
        ApiV2ServerConfig
    >;
    
    Variant data;
    
    // Все операции - методы структуры
    QString description() const;
    QString hostName() const;
    QMap<DockerContainer, ContainerConfig> containers() const;
    DockerContainer defaultContainer() const;
    QString dns1() const;
    QString dns2() const;
    bool hasContainers() const;
    ContainerConfig containerConfig(DockerContainer container) const;
    int crc() const;
    int configVersion() const;
    QJsonObject toJson() const;
    
    // Type checks
    bool isSelfHosted() const;
    bool isNative() const;
    bool isApiV1() const;
    bool isApiV2() const;
    bool isApiConfig() const;
    
    // Type access (опционально, для специфичных полей)
    template<typename T>
    T* as() { return std::get_if<T>(&data); }
    
    template<typename T>
    const T* as() const { return std::get_if<T>(&data); }
    
    // Factory
    static ServerConfig fromJson(const QJsonObject& json);
};
```

**Реализация методов:**
```cpp
QString ServerConfig::description() const {
    return std::visit([](const auto& v) { return v.description; }, data);
}

QString ServerConfig::hostName() const {
    return std::visit([](const auto& v) { return v.hostName; }, data);
}

bool ServerConfig::isApiV1() const {
    return std::holds_alternative<ApiV1ServerConfig>(data);
}

bool ServerConfig::isApiV2() const {
    return std::holds_alternative<ApiV2ServerConfig>(data);
}
```

---

## Преимущества для реального кода

### До рефакторинга:
```cpp
// serversModel.cpp
QString name = ServerConfigUtils::description(server);
QString host = ServerConfigUtils::hostName(server);
DockerContainer container = ServerConfigUtils::defaultContainer(server);
if (ServerConfigUtils::isApiV2Config(server)) {
    return ServerConfigUtils::asApiV2(server).apiConfig.availableCountries;
}

// installController.cpp
QString port = ProtocolConfigUtils::portWithDefault(config.protocolConfig, protocol);
bool hasClient = ProtocolConfigUtils::hasClientConfig(containerConfig.protocolConfig);
```

### После рефакторинга:
```cpp
// serversModel.cpp
QString name = server.description();
QString host = server.hostName();
DockerContainer container = server.defaultContainer();
if (server.isApiV2()) {
    return server.as<ApiV2ServerConfig>()->apiConfig.availableCountries;
}

// installController.cpp
QString port = config.protocol.portWithDefault(protocol);
bool hasClient = config.protocol.hasClientConfig();
```

**Улучшения:**
- ✅ Код короче и понятнее
- ✅ Нет префикса `ServerConfigUtils::` / `ProtocolConfigUtils::`
- ✅ Инкапсуляция операций
- ✅ Сохраняется value semantics
- ✅ Нет heap-аллокаций

---

## План миграции

### Этап 1: ProtocolConfig (1 неделя)

1. **Создать структуру `ProtocolConfig`** с полем `Variant data`
2. **Перенести методы из `ProtocolConfigUtils`** в структуру
3. **Обновить `ContainerConfig`** для использования `protocol.type()`, `protocol.toJson()` вместо утилит
4. **Обновить все использования:**
   - `ProtocolConfigUtils::port(config)` → `config.port()`
   - `ProtocolConfigUtils::hasClientConfig(config)` → `config.hasClientConfig()`
   - И т.д.

**Файлы для изменения:**
- `core/models/protocolConfig.h` / `.cpp` - создать структуру
- `core/models/containerConfig.cpp` - использовать методы
- `core/controllers/selfhosted/installController.cpp` - обновить вызовы
- `core/controllers/selfhosted/exportController.cpp` - обновить вызовы
- `ui/models/containersModel.cpp` - обновить вызовы
- `core/repositories/secureServersRepository.cpp` - обновить вызовы

### Этап 2: ServerConfig (1 неделя)

1. **Создать структуру `ServerConfig`** с полем `Variant data`
2. **Перенести методы из `ServerConfigUtils`** в структуру
3. **Обновить все использования:**
   - `ServerConfigUtils::description(server)` → `server.description()`
   - `ServerConfigUtils::isApiV1Config(server)` → `server.isApiV1()`
   - И т.д.

**Файлы для изменения:**
- `core/models/serverConfig.h` / `.cpp` - создать структуру
- `ui/models/serversModel.cpp` - обновить вызовы (много мест)
- `ui/controllers/serversUiController.cpp` - обновить вызовы
- `core/controllers/connectionController.cpp` - обновить вызовы
- `core/controllers/selfhosted/installController.cpp` - обновить вызовы
- `core/repositories/secureServersRepository.cpp` - обновить вызовы
- Все тесты

### Этап 3: Очистка (3-5 дней)

1. **Удалить `ProtocolConfigUtils`** и `ServerConfigUtils`
2. **Обновить тесты** - заменить вызовы утилит на методы
3. **Обновить документацию**

---

## Обратная совместимость

Для плавной миграции можно временно оставить утилиты как обертки:

```cpp
// Временно, для обратной совместимости
namespace ProtocolConfigUtils {
    QString port(const ProtocolConfig& config) {
        return config.port();  // Делегирует методу
    }
    // ... остальные методы
}
```

После миграции всех мест использования - удалить утилиты.

---

## Сравнение подходов

| Критерий | Текущий (Utils) | Полиморфизм (virtual) | Рекомендуемый (методы) |
|----------|-----------------|----------------------|------------------------|
| Value semantics | ✅ | ❌ | ✅ |
| Инкапсуляция | ❌ | ✅ | ✅ |
| Heap-аллокации | ✅ Нет | ❌ Есть | ✅ Нет |
| Exhaustiveness checking | ❌ | ❌ | ✅ |
| Простота отладки | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| Объём изменений | - | Большой | Средний |
| Производительность | Отлично | Хорошо | Отлично |
| Читаемость кода | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |

---

## Итог

**Проблема:** Операции над структурными данными вынесены в утилиты, что размазывает логику.

**Решение:** Инкапсулировать операции в методы структур, оставив `variant` для данных.

**Результат:**
- ✅ Инкапсуляция без потери value semantics
- ✅ Упрощение кода без потери производительности
- ✅ Лучшая читаемость
- ✅ Минимальный объём изменений (средний, не большой)

**Полиморфизм не нужен**, так как это структурные данные, а не поведенческая иерархия.
