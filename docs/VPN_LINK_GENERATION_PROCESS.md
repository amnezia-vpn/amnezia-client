# Процесс генерации VPN ссылок (vpn://) в Amnezia Client

## Обзор

Amnezia Client использует специальный формат ссылок `vpn://` для обмена конфигурациями VPN. Этот документ описывает, как создаются эти ссылки и как можно преобразовать обычную конфигурацию WireGuard в ссылку формата `vpn://`.

## Как работает генерация ссылок

### Процесс кодирования (Config → vpn:// link)

Код находится в файле: `/client/ui/controllers/exportController.cpp`

```cpp
// Строки 52-54, 94-96
QByteArray compressedConfig = QJsonDocument(serverConfig).toJson();
compressedConfig = qCompress(compressedConfig, 8);
m_config = QString("vpn://%1").arg(QString(compressedConfig.toBase64(
    QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)));
```

**Шаги:**
1. **Сериализация в JSON**: Конфигурация сервера преобразуется в JSON
2. **Сжатие zlib**: JSON сжимается с использованием `qCompress()` (уровень сжатия 8)
3. **Base64 URL-safe кодирование**: Сжатые данные кодируются в Base64 с флагами:
   - `Base64UrlEncoding` - URL-безопасное кодирование (использует `-` и `_` вместо `+` и `/`)
   - `OmitTrailingEquals` - убирает завершающие символы `=`
4. **Добавление префикса**: К результату добавляется префикс `vpn://`

### Процесс декодирования (vpn:// link → Config)

Код находится в файле: `/client/ui/controllers/importController.cpp`

```cpp
// Строки 156-161
config.replace("vpn://", "");
QByteArray ba = QByteArray::fromBase64(config.toUtf8(), 
    QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
QByteArray baUncompressed = qUncompress(ba);
if (!baUncompressed.isEmpty()) {
    ba = baUncompressed;
}
```

**Шаги:**
1. **Удаление префикса**: Убирается `vpn://`
2. **Base64 декодирование**: URL-safe Base64 декодируется
3. **Распаковка zlib**: Данные распаковываются с помощью `qUncompress()`
4. **Парсинг JSON**: Результат парсится как JSON конфигурация

## Формат конфигурации Amnezia

Конфигурация в Amnezia - это **НЕ просто WireGuard config**, а специальный JSON со структурой сервера. Формат включает:

```json
{
  "hostName": "89.125.213.14",
  "defaultContainer": "amnezia-awg",
  "dns1": "1.1.1.1",
  "dns2": "8.8.8.8",
  "containers": [
    {
      "container": "amnezia-awg",
      "awg": {
        "port": "46811",
        "client_priv_key": "QI1ESrtAWzg4I6M8v8roRRdqldRCosjR6zpgFp1FRnM=",
        "client_pub_key": "...",
        "client_ip": "10.8.1.2/24",
        "psk_key": "yaYGl/gM1vNml0ST+RWkAQnc3+eC9iZ9TPyz3jvuIFc=",
        "server_pub_key": "ARATMWdjtitj3/MO8tCq7mMA7XL84SucUq+mKccNsTs=",
        "Jc": 6,
        "Jmin": 10,
        "Jmax": 50,
        "S1": 123,
        "S2": 136,
        "H1": 1043813656,
        "H2": 1394807736,
        "H3": 850386757,
        "H4": 714960491
      }
    }
  ]
}
```

## Почему простой Base64 не работает?

Простое кодирование WireGuard конфига в Base64 не работает по следующим причинам:

1. **Формат данных**: Amnezia использует JSON структуру, а не текстовый INI формат WireGuard
2. **Сжатие**: Данные сжимаются с помощью zlib перед кодированием
3. **URL-safe Base64**: Используется специальный вариант Base64 без `=` в конце
4. **Структура контейнера**: Нужно обернуть конфигурацию в структуру с контейнером и метаданными сервера

## Как создать vpn:// ссылку из WireGuard конфига

### Вариант 1: Использование Qt (C++)

```cpp
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>

QString wireguardConfigToVpnLink(const QString& privateKey, 
                                  const QString& address,
                                  const QString& publicKey,
                                  const QString& presharedKey,
                                  const QString& endpoint,
                                  const QString& dns1,
                                  const QString& dns2,
                                  int jc, int jmin, int jmax,
                                  int s1, int s2,
                                  qint64 h1, qint64 h2, qint64 h3, qint64 h4)
{
    // Парсим endpoint
    QStringList endpointParts = endpoint.split(":");
    QString hostName = endpointParts[0];
    QString port = endpointParts.size() > 1 ? endpointParts[1] : "51820";
    
    // Парсим IP адрес (убираем /24)
    QString clientIp = address;
    
    // Создаем JSON структуру
    QJsonObject awgConfig;
    awgConfig["port"] = port;
    awgConfig["client_priv_key"] = privateKey;
    awgConfig["client_ip"] = clientIp;
    awgConfig["psk_key"] = presharedKey;
    awgConfig["server_pub_key"] = publicKey;
    awgConfig["Jc"] = jc;
    awgConfig["Jmin"] = jmin;
    awgConfig["Jmax"] = jmax;
    awgConfig["S1"] = s1;
    awgConfig["S2"] = s2;
    awgConfig["H1"] = QString::number(h1);
    awgConfig["H2"] = QString::number(h2);
    awgConfig["H3"] = QString::number(h3);
    awgConfig["H4"] = QString::number(h4);
    
    QJsonObject containerConfig;
    containerConfig["container"] = "amnezia-awg";
    containerConfig["awg"] = awgConfig;
    
    QJsonObject serverConfig;
    serverConfig["hostName"] = hostName;
    serverConfig["defaultContainer"] = "amnezia-awg";
    serverConfig["dns1"] = dns1;
    serverConfig["dns2"] = dns2;
    serverConfig["containers"] = QJsonArray({ containerConfig });
    
    // Сериализация, сжатие и кодирование
    QByteArray jsonData = QJsonDocument(serverConfig).toJson(QJsonDocument::Compact);
    QByteArray compressed = qCompress(jsonData, 8);
    QString base64 = QString(compressed.toBase64(
        QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    
    return QString("vpn://%1").arg(base64);
}
```

### Вариант 2: Использование Python

```python
import json
import zlib
import base64

def wireguard_config_to_vpn_link(
    private_key, address, public_key, preshared_key, 
    endpoint, dns1, dns2,
    jc, jmin, jmax, s1, s2, h1, h2, h3, h4
):
    # Парсим endpoint
    host_name, port = endpoint.split(':')
    
    # Создаем JSON структуру
    server_config = {
        "hostName": host_name,
        "defaultContainer": "amnezia-awg",
        "dns1": dns1,
        "dns2": dns2,
        "containers": [
            {
                "container": "amnezia-awg",
                "awg": {
                    "port": port,
                    "client_priv_key": private_key,
                    "client_ip": address,
                    "psk_key": preshared_key,
                    "server_pub_key": public_key,
                    "Jc": jc,
                    "Jmin": jmin,
                    "Jmax": jmax,
                    "S1": s1,
                    "S2": s2,
                    "H1": str(h1),
                    "H2": str(h2),
                    "H3": str(h3),
                    "H4": str(h4)
                }
            }
        ]
    }
    
    # Сериализуем в JSON (компактный формат)
    json_data = json.dumps(server_config, separators=(',', ':')).encode('utf-8')
    
    # Сжимаем с zlib (уровень 8)
    compressed = zlib.compress(json_data, 8)
    
    # Кодируем в URL-safe Base64 без padding
    base64_encoded = base64.urlsafe_b64encode(compressed).decode('ascii').rstrip('=')
    
    return f"vpn://{base64_encoded}"

# Пример использования с вашей конфигурацией
vpn_link = wireguard_config_to_vpn_link(
    private_key="QI1ESrtAWzg4I6M8v8roRRdqldRCosjR6zpgFp1FRnM=",
    address="10.8.1.2/24",
    public_key="ARATMWdjtitj3/MO8tCq7mMA7XL84SucUq+mKccNsTs=",
    preshared_key="yaYGl/gM1vNml0ST+RWkAQnc3+eC9iZ9TPyz3jvuIFc=",
    endpoint="89.125.213.14:46811",
    dns1="1.1.1.1",
    dns2="8.8.8.8",
    jc=6, jmin=10, jmax=50,
    s1=123, s2=136,
    h1=1043813656, h2=1394807736, h3=850386757, h4=714960491
)

print(vpn_link)
```

### Вариант 3: Использование Bash/Shell

```bash
#!/bin/bash

# Функция для создания vpn:// ссылки
create_vpn_link() {
    # Параметры из WireGuard конфига
    PRIVATE_KEY="QI1ESrtAWzg4I6M8v8roRRdqldRCosjR6zpgFp1FRnM="
    ADDRESS="10.8.1.2/24"
    DNS1="1.1.1.1"
    DNS2="8.8.8.8"
    PUBLIC_KEY="ARATMWdjtitj3/MO8tCq7mMA7XL84SucUq+mKccNsTs="
    PRESHARED_KEY="yaYGl/gM1vNml0ST+RWkAQnc3+eC9iZ9TPyz3jvuIFc="
    ENDPOINT="89.125.213.14:46811"
    JC=6
    JMIN=10
    JMAX=50
    S1=123
    S2=136
    H1=1043813656
    H2=1394807736
    H3=850386757
    H4=714960491
    
    # Парсим endpoint
    HOST=$(echo $ENDPOINT | cut -d: -f1)
    PORT=$(echo $ENDPOINT | cut -d: -f2)
    
    # Создаем JSON (компактный формат, без пробелов)
    JSON=$(cat <<EOF
{"hostName":"$HOST","defaultContainer":"amnezia-awg","dns1":"$DNS1","dns2":"$DNS2","containers":[{"container":"amnezia-awg","awg":{"port":"$PORT","client_priv_key":"$PRIVATE_KEY","client_ip":"$ADDRESS","psk_key":"$PRESHARED_KEY","server_pub_key":"$PUBLIC_KEY","Jc":$JC,"Jmin":$JMIN,"Jmax":$JMAX,"S1":$S1,"S2":$S2,"H1":"$H1","H2":"$H2","H3":"$H3","H4":"$H4"}}]}
EOF
)
    
    # Сжимаем с помощью Python (требуется Python)
    COMPRESSED=$(echo -n "$JSON" | python3 -c "
import sys, zlib, base64
data = sys.stdin.buffer.read()
compressed = zlib.compress(data, 8)
encoded = base64.urlsafe_b64encode(compressed).decode('ascii').rstrip('=')
print('vpn://' + encoded)
")
    
    echo "$COMPRESSED"
}

# Выполняем
create_vpn_link
```

## Декодирование vpn:// ссылки

### Python пример декодирования:

```python
import json
import zlib
import base64

def decode_vpn_link(vpn_link):
    # Убираем префикс vpn://
    encoded = vpn_link.replace('vpn://', '')
    
    # Добавляем padding если нужно
    padding = 4 - (len(encoded) % 4)
    if padding != 4:
        encoded += '=' * padding
    
    # Декодируем из URL-safe Base64
    compressed = base64.urlsafe_b64decode(encoded)
    
    # Распаковываем zlib
    json_data = zlib.decompress(compressed)
    
    # Парсим JSON
    config = json.loads(json_data)
    
    return config

# Пример использования
vpn_link = "vpn://eJy1k9tuozAQhu_7FCHXu5HNIYfeUQ5JIDbhnLBaVQ5k04SjAjRA1XdfQ9Kq2YtdJct8_z-jmbF5fxiNmDDPKnLM9ueSeRz9omQ0eh_27xqVGJJm-_5Ifp..."
config = decode_vpn_link(vpn_link)
print(json.dumps(config, indent=2))
```

## Важные замечания

1. **AmneziaWG параметры**: Параметры `Jc`, `Jmin`, `Jmax`, `S1`, `S2`, `H1-H4` - это специфичные для AmneziaWG параметры обфускации
2. **Тип контейнера**: Используйте `"amnezia-awg"` для AmneziaWG или `"amnezia-wg"` для обычного WireGuard
3. **DNS серверы**: Указываются отдельно в корневом объекте JSON
4. **Сжатие обязательно**: Без zlib сжатия ссылка не будет работать
5. **URL-safe Base64**: Обычный Base64 не подойдет, нужен именно URL-safe вариант

## Ссылки на исходный код

- Генерация ссылок: `/client/ui/controllers/exportController.cpp` (строки 52-54, 94-96)
- Импорт ссылок: `/client/ui/controllers/importController.cpp` (строки 156-161)
- Конфигуратор WireGuard: `/client/configurators/wireguard_configurator.cpp`
- Утилиты для QR кодов: `/client/core/qrCodeUtils.cpp`
