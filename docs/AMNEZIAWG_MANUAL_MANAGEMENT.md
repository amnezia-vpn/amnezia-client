# Руководство по управлению пользователями AmneziaWG через SSH

AmneziaWG - это обфусцированная версия WireGuard с функциями антиDPI для обхода блокировок. Управление пользователями осуществляется через SSH подключение к серверу.

## Содержание
- [Подключение к серверу](#подключение-к-серверу)
- [Добавление нового пользователя](#добавление-нового-пользователя)
- [Просмотр данных для подключения](#просмотр-данных-для-подключения)
- [Отзыв (удаление) пользователя](#отзыв-удаление-пользователя)
- [Просмотр активных подключений](#просмотр-активных-подключений)
- [Полезные команды](#полезные-команды)

---

## Подключение к серверу

### SSH подключение
```bash
# Подключение к серверу
ssh root@ваш-сервер-ip

# Или с использованием SSH ключа
ssh -i /путь/к/ключу root@ваш-сервер-ip
```

### Проверка контейнера AmneziaWG
```bash
# Убедитесь, что контейнер запущен
docker ps --filter name=amnezia-awg

# Вывод должен показать:
# CONTAINER ID   IMAGE          STATUS         PORTS
# abc123...      amnezia-awg    Up 2 days      0.0.0.0:51820->51820/udp
```

---

## Добавление нового пользователя

### Шаг 1: Войти в контейнер AmneziaWG
```bash
docker exec -it amnezia-awg bash
```

### Шаг 2: Сгенерировать ключи для нового пользователя
```bash
# Перейти во временную директорию
cd /tmp

# Сгенерировать приватный и публичный ключи клиента
awg genkey | tee client_private.key | awg pubkey > client_public.key

# Сгенерировать предварительно распределённый ключ (preshared key)
awg genpsk > client_preshared.key

# Посмотреть сгенерированные ключи
echo "Приватный ключ клиента:"
cat client_private.key

echo "Публичный ключ клиента:"
cat client_public.key

echo "Preshared ключ:"
cat client_preshared.key
```

**Пример вывода:**
```
Приватный ключ клиента:
yAnz5TF+lXXJte14tji3zlMNq+hd2rYUIgJBgB3fBmk=

Публичный ключ клиента:
HIgo9xNzJMWLKASShiTqIybxZ0U3wGLiUeHV6U2v220=

Preshared ключ:
FBqnfJNKcIE8VVf+UJxKLCvFPZ4IJKJYyaJKfH6pJAE=
```

**ВАЖНО:** Сохраните эти ключи! Они понадобятся для создания конфигурации клиента.

### Шаг 3: Добавить peer в конфигурацию сервера
```bash
# Получить значения ключей в переменные
CLIENT_PUBLIC_KEY=$(cat client_public.key)
CLIENT_PRESHARED_KEY=$(cat client_preshared.key)

# Определить IP-адрес для нового клиента
# Смотрим, какие IP уже используются
grep "AllowedIPs" /opt/amnezia/awg/awg0.conf

# Выбираем следующий свободный IP (например, 10.8.1.10)
# Если это первый клиент после установки, используйте 10.8.1.2

# Добавить секцию [Peer] в конфигурацию
cat >> /opt/amnezia/awg/awg0.conf << EOF

[Peer]
# Имя пользователя (комментарий для удобства)
PublicKey = $CLIENT_PUBLIC_KEY
PresharedKey = $CLIENT_PRESHARED_KEY
AllowedIPs = 10.8.1.10/32
EOF
```

### Шаг 4: Применить конфигурацию без перезапуска
```bash
# Применить изменения к работающему интерфейсу
awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)

# Проверить, что peer добавлен
awg show

# Вывод покажет новый peer:
# peer: HIgo9xNzJMWLKASShiTqIybxZ0U3wGLiUeHV6U2v220=
#   preshared key: (hidden)
#   allowed ips: 10.8.1.10/32
```

### Шаг 5: Обновить clientsTable (для совместимости с Amnezia клиентом)
```bash
# Редактировать файл со списком клиентов
vi /opt/amnezia/awg/clientsTable

# Добавить запись в JSON массив:
# [
#   {
#     "clientId": "HIgo9xNzJMWLKASShiTqIybxZ0U3wGLiUeHV6U2v220=",
#     "userData": {
#       "clientName": "Имя Пользователя",
#       "creationDate": "2024-01-25T15:00:00"
#     }
#   }
# ]

# Сохранить и выйти (:wq в vi)
```

### Шаг 6: Выйти из контейнера
```bash
exit
```

---

## Просмотр данных для подключения

После добавления пользователя нужно создать конфигурационный файл для клиента.

### Получить параметры сервера
```bash
# Войти в контейнер
docker exec -it amnezia-awg bash

# Получить публичный ключ сервера
cat /opt/amnezia/awg/wireguard_server_public_key.key

# Получить параметры обфускации из конфигурации сервера
grep -E "^(Jc|Jmin|Jmax|S1|S2|S3|S4|H1|H2|H3|H4|I1|I2|I3|I4|I5)" /opt/amnezia/awg/awg0.conf

# Выйти
exit
```

**Пример вывода:**
```
Публичный ключ сервера:
2bXVEJz1v5DkZ9w7r9NQpPHb0Q3C7M8iO6VexLLG0XE=

Параметры обфускации:
Jc = 5
Jmin = 10
Jmax = 50
S1 = 85
S2 = 142
S3 = 28
S4 = 15
H1 = 1234567-9876543
H2 = 2345678-8765432
H3 = 3456789-7654321
H4 = 4567890-6543210
```

### Создать конфигурационный файл для клиента

Создайте файл `client-amneziawg.conf` на локальной машине с следующим содержимым:

```ini
[Interface]
# Приватный ключ клиента (из client_private.key)
PrivateKey = yAnz5TF+lXXJte14tji3zlMNq+hd2rYUIgJBgB3fBmk=

# IP-адрес клиента в VPN (тот же, что в AllowedIPs на сервере)
Address = 10.8.1.10/32

# DNS серверы
DNS = 1.1.1.1, 1.0.0.1

# Параметры обфускации (скопировать с сервера)
Jc = 5
Jmin = 10
Jmax = 50
S1 = 85
S2 = 142
S3 = 28
S4 = 15
H1 = 1234567-9876543
H2 = 2345678-8765432
H3 = 3456789-7654321
H4 = 4567890-6543210
I1 = 0
I2 = 0
I3 = 0
I4 = 0
I5 = 0

[Peer]
# Публичный ключ сервера
PublicKey = 2bXVEJz1v5DkZ9w7r9NQpPHb0Q3C7M8iO6VexLLG0XE=

# Preshared ключ (из client_preshared.key)
PresharedKey = FBqnfJNKcIE8VVf+UJxKLCvFPZ4IJKJYyaJKfH6pJAE=

# IP-адрес и порт сервера
Endpoint = ваш-сервер-ip:51820

# Маршрутизировать весь трафик через VPN
AllowedIPs = 0.0.0.0/0, ::/0

# Keepalive для поддержания соединения через NAT
PersistentKeepalive = 25
```

**Этот файл нужно передать пользователю** для импорта в Amnezia VPN клиент или использования с AmneziaWG напрямую.

### Альтернатива: Скрипт для автоматического создания конфигурации

```bash
# На сервере создайте скрипт для генерации конфигурации клиента
docker exec -it amnezia-awg bash

cat > /tmp/create_client_config.sh << 'SCRIPT'
#!/bin/bash

# Параметры
CLIENT_NAME=$1
CLIENT_IP=$2

if [ -z "$CLIENT_NAME" ] || [ -z "$CLIENT_IP" ]; then
    echo "Использование: $0 <имя_клиента> <ip_клиента>"
    echo "Пример: $0 alice 10.8.1.10"
    exit 1
fi

# Генерация ключей
cd /tmp
awg genkey | tee ${CLIENT_NAME}_private.key | awg pubkey > ${CLIENT_NAME}_public.key
awg genpsk > ${CLIENT_NAME}_preshared.key

CLIENT_PRIVATE_KEY=$(cat ${CLIENT_NAME}_private.key)
CLIENT_PUBLIC_KEY=$(cat ${CLIENT_NAME}_public.key)
CLIENT_PRESHARED_KEY=$(cat ${CLIENT_NAME}_preshared.key)

# Получение данных сервера
SERVER_PUBLIC_KEY=$(cat /opt/amnezia/awg/wireguard_server_public_key.key)
SERVER_IP=$(grep "Endpoint" /opt/amnezia/awg/awg0.conf | head -1 | cut -d'=' -f2 | xargs || echo "SERVER_IP:51820")
AWG_PORT=$(grep "ListenPort" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)

# Получение параметров обфускации
JC=$(grep "^Jc" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
JMIN=$(grep "^Jmin" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
JMAX=$(grep "^Jmax" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
S1=$(grep "^S1" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
S2=$(grep "^S2" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
S3=$(grep "^S3" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
S4=$(grep "^S4" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
H1=$(grep "^H1" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
H2=$(grep "^H2" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
H3=$(grep "^H3" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
H4=$(grep "^H4" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)

# Добавление peer в конфигурацию сервера
cat >> /opt/amnezia/awg/awg0.conf << EOF

[Peer]
# ${CLIENT_NAME}
PublicKey = ${CLIENT_PUBLIC_KEY}
PresharedKey = ${CLIENT_PRESHARED_KEY}
AllowedIPs = ${CLIENT_IP}/32
EOF

# Применение конфигурации
awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)

# Создание конфигурации клиента
cat > /tmp/${CLIENT_NAME}_amneziawg.conf << EOF
[Interface]
PrivateKey = ${CLIENT_PRIVATE_KEY}
Address = ${CLIENT_IP}/32
DNS = 1.1.1.1, 1.0.0.1
Jc = ${JC}
Jmin = ${JMIN}
Jmax = ${JMAX}
S1 = ${S1}
S2 = ${S2}
S3 = ${S3}
S4 = ${S4}
H1 = ${H1}
H2 = ${H2}
H3 = ${H3}
H4 = ${H4}
I1 = 0
I2 = 0
I3 = 0
I4 = 0
I5 = 0

[Peer]
PublicKey = ${SERVER_PUBLIC_KEY}
PresharedKey = ${CLIENT_PRESHARED_KEY}
Endpoint = ${SERVER_IP}:${AWG_PORT}
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 25
EOF

echo "====================================="
echo "Клиент ${CLIENT_NAME} успешно добавлен!"
echo "====================================="
echo "Публичный ключ: ${CLIENT_PUBLIC_KEY}"
echo "IP-адрес: ${CLIENT_IP}"
echo ""
echo "Конфигурационный файл создан: /tmp/${CLIENT_NAME}_amneziawg.conf"
echo ""
echo "Чтобы скачать конфигурацию на локальную машину:"
echo "docker cp amnezia-awg:/tmp/${CLIENT_NAME}_amneziawg.conf ./"
SCRIPT

chmod +x /tmp/create_client_config.sh

# Использование скрипта
/tmp/create_client_config.sh alice 10.8.1.10

# Выйти из контейнера
exit

# Скачать конфигурацию на локальную машину
docker cp amnezia-awg:/tmp/alice_amneziawg.conf ./
```

---

## Отзыв (удаление) пользователя

### Шаг 1: Войти в контейнер
```bash
docker exec -it amnezia-awg bash
```

### Шаг 2: Определить публичный ключ пользователя для удаления
```bash
# Просмотреть текущих peer в конфигурации
cat /opt/amnezia/awg/awg0.conf

# Или посмотреть активные подключения
awg show

# Найдите секцию [Peer] с нужным пользователем
```

### Шаг 3: Удалить секцию [Peer] из конфигурации
```bash
# Открыть конфигурацию в редакторе
vi /opt/amnezia/awg/awg0.conf

# Найти и удалить секцию [Peer] для пользователя
# Пример:
# [Peer]
# # alice
# PublicKey = HIgo9xNzJMWLKASShiTqIybxZ0U3wGLiUeHV6U2v220=
# PresharedKey = FBqnfJNKcIE8VVf+UJxKLCvFPZ4IJKJYyaJKfH6pJAE=
# AllowedIPs = 10.8.1.10/32

# Удалите всю эту секцию, включая комментарий
# Сохраните файл (:wq в vi)
```

### Шаг 4: Применить изменения
```bash
# Применить конфигурацию без перезапуска
awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)

# Проверить, что peer удален
awg show

# Пользователь больше не должен отображаться в списке
```

### Шаг 5: Обновить clientsTable
```bash
# Редактировать файл со списком клиентов
vi /opt/amnezia/awg/clientsTable

# Найти и удалить запись с публичным ключом пользователя
# Сохранить и выйти (:wq)
```

### Шаг 6: Выйти из контейнера
```bash
exit
```

**Результат:** Пользователь немедленно отключится от VPN и больше не сможет подключиться.

---

## Просмотр активных подключений

### Показать статус всех подключений
```bash
docker exec amnezia-awg awg show

# Вывод покажет:
# interface: awg0
#   public key: 2bXVEJz1v5DkZ9w7r9NQpPHb0Q3C7M8iO6VexLLG0XE=
#   private key: (hidden)
#   listening port: 51820
#
# peer: HIgo9xNzJMWLKASShiTqIybxZ0U3wGLiUeHV6U2v220=
#   preshared key: (hidden)
#   endpoint: 95.123.45.67:54321
#   allowed ips: 10.8.1.10/32
#   latest handshake: 2 minutes, 15 seconds ago
#   transfer: 150.5 MiB received, 45.2 MiB sent
#   persistent keepalive: every 25 seconds
```

### Интерпретация вывода:
- **peer**: Публичный ключ клиента
- **endpoint**: Реальный IP-адрес и порт клиента
- **allowed ips**: Внутренний IP-адрес в VPN
- **latest handshake**: Время последнего обмена ключами (показывает активность)
- **transfer**: Объем переданных данных (received = скачано клиентом, sent = загружено клиентом)

### Показать только подключенных пользователей
```bash
docker exec amnezia-awg bash -c "awg show | grep -A 6 'peer:'"

# Или отфильтровать по недавней активности (handshake < 5 минут)
docker exec amnezia-awg bash -c "awg show all dump | awk '\$6 < 300 {print \$1, \$6}'"
```

---

## Полезные команды

### Просмотр конфигурации сервера
```bash
docker exec amnezia-awg cat /opt/amnezia/awg/awg0.conf
```

### Просмотр списка всех клиентов
```bash
docker exec amnezia-awg cat /opt/amnezia/awg/clientsTable
```

### Подсчет количества клиентов
```bash
docker exec amnezia-awg bash -c "grep -c '^\[Peer\]' /opt/amnezia/awg/awg0.conf"
```

### Список IP-адресов всех клиентов
```bash
docker exec amnezia-awg bash -c "grep 'AllowedIPs' /opt/amnezia/awg/awg0.conf | grep -v '^#'"
```

### Проверка статуса контейнера
```bash
docker ps --filter name=amnezia-awg
docker logs --tail 50 amnezia-awg
```

### Перезапуск контейнера (если нужно)
```bash
docker restart amnezia-awg
```

### Резервное копирование конфигурации
```bash
# Создать backup
docker exec amnezia-awg tar -czf /tmp/awg-backup.tar.gz /opt/amnezia/awg/

# Скачать на локальную машину
docker cp amnezia-awg:/tmp/awg-backup.tar.gz ./awg-backup-$(date +%Y%m%d).tar.gz
```

### Восстановление из backup
```bash
# Загрузить backup на сервер
docker cp awg-backup-20240125.tar.gz amnezia-awg:/tmp/

# Восстановить
docker exec amnezia-awg bash -c "tar -xzf /tmp/awg-backup-20240125.tar.gz -C /"

# Применить конфигурацию
docker exec amnezia-awg bash -c "awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)"
```

---

## Диагностика проблем

### Клиент не может подключиться

1. **Проверить, слушает ли порт:**
```bash
netstat -tuln | grep 51820
# Или
ss -tuln | grep 51820
```

2. **Проверить логи контейнера:**
```bash
docker logs --tail 100 amnezia-awg
```

3. **Проверить firewall:**
```bash
iptables -L -n -v | grep 51820
```

4. **Проверить NAT:**
```bash
iptables -t nat -L -n -v
```

### Клиент подключен, но нет интернета

1. **Проверить маршрутизацию:**
```bash
docker exec amnezia-awg ip route
```

2. **Проверить forwarding:**
```bash
docker exec amnezia-awg cat /proc/sys/net/ipv4/ip_forward
# Должно быть 1
```

3. **Проверить MASQUERADE:**
```bash
iptables -t nat -L POSTROUTING -n -v
```

---

## Важные замечания

1. **Параметры обфускации** (Jc, Jmin, Jmax, S1-S4, H1-H4, I1-I5) должны быть **одинаковыми** на сервере и клиенте. Если они не совпадают, соединение не установится.

2. **Публичный ключ клиента** уникален для каждого пользователя. Используйте его как идентификатор в clientsTable.

3. **IP-адреса клиентов** должны быть уникальными в пределах подсети (обычно 10.8.1.0/24). Не назначайте один IP двум разным клиентам.

4. **Команда `awg syncconf`** применяет изменения **без** перезапуска VPN сервиса, что означает, что активные подключения не разрываются.

5. **PresharedKey** добавляет дополнительный уровень безопасности post-quantum криптографией. Рекомендуется для всех клиентов.

6. **Резервные копии** конфигурации делайте регулярно, особенно перед массовым добавлением/удалением пользователей.

---

## Дополнительные ресурсы

- [Документация WireGuard](https://www.wireguard.com/)
- [AmneziaWG GitHub](https://github.com/amnezia-vpn/amneziawg-linux-kernel-module)
- [Документация Amnezia VPN](https://docs.amnezia.org/)

---

**Автор:** Документация создана на основе анализа исходного кода Amnezia Client  
**Дата:** 2024-01-25  
**Версия:** 1.0
