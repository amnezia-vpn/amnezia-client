# Быстрая справка по AmneziaWG

## Содержание
- [Добавление пользователя](#добавление-пользователя)
- [Отзыв пользователя](#отзыв-пользователя)
- [Просмотр подключений](#просмотр-подключений)
- [Изменение параметров обфускации](#изменение-параметров-обфускации)
- [Смена порта](#смена-порта)

---

## Добавление пользователя

### Быстрый способ (вручную)
```bash
# 1. Войти в контейнер
docker exec -it amnezia-awg bash

# 2. Сгенерировать ключи
cd /tmp
awg genkey | tee client_private.key | awg pubkey > client_public.key
awg genpsk > client_preshared.key

# Сохранить значения
CLIENT_PRIVATE=$(cat client_private.key)
CLIENT_PUBLIC=$(cat client_public.key)
CLIENT_PSK=$(cat client_preshared.key)

# 3. Добавить в конфигурацию сервера
cat >> /opt/amnezia/awg/awg0.conf << EOF

[Peer]
# Имя пользователя
PublicKey = $CLIENT_PUBLIC
PresharedKey = $CLIENT_PSK
AllowedIPs = 10.8.1.10/32
EOF

# 4. Применить изменения
awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)

# 5. Проверить
awg show

exit
```

### Автоматический способ (скрипт)
```bash
docker exec -it amnezia-awg bash

# Создать скрипт (один раз)
cat > /usr/local/bin/add_awg_user << 'EOF'
#!/bin/bash
CLIENT_NAME=$1
CLIENT_IP=$2

if [ -z "$CLIENT_NAME" ] || [ -z "$CLIENT_IP" ]; then
    echo "Использование: add_awg_user <имя> <ip>"
    echo "Пример: add_awg_user alice 10.8.1.10"
    exit 1
fi

cd /tmp
awg genkey | tee ${CLIENT_NAME}_private.key | awg pubkey > ${CLIENT_NAME}_public.key
awg genpsk > ${CLIENT_NAME}_preshared.key

CLIENT_PRIVATE=$(cat ${CLIENT_NAME}_private.key)
CLIENT_PUBLIC=$(cat ${CLIENT_NAME}_public.key)
CLIENT_PSK=$(cat ${CLIENT_NAME}_preshared.key)

cat >> /opt/amnezia/awg/awg0.conf << EOFPEER

[Peer]
# ${CLIENT_NAME}
PublicKey = ${CLIENT_PUBLIC}
PresharedKey = ${CLIENT_PSK}
AllowedIPs = ${CLIENT_IP}/32
EOFPEER

awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)

# Получить параметры сервера
SERVER_PUBLIC=$(cat /opt/amnezia/awg/wireguard_server_public_key.key)
SERVER_IP=$(hostname -I | awk '{print $1}')
AWG_PORT=$(grep "ListenPort" /opt/amnezia/awg/awg0.conf | cut -d'=' -f2 | xargs)
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

# Создать конфигурацию клиента
cat > /tmp/${CLIENT_NAME}_amneziawg.conf << EOFCLIENT
[Interface]
PrivateKey = ${CLIENT_PRIVATE}
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
PublicKey = ${SERVER_PUBLIC}
PresharedKey = ${CLIENT_PSK}
Endpoint = ${SERVER_IP}:${AWG_PORT}
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 25
EOFCLIENT

echo "====================================="
echo "Пользователь ${CLIENT_NAME} добавлен!"
echo "====================================="
echo "Публичный ключ: ${CLIENT_PUBLIC}"
echo "IP: ${CLIENT_IP}"
echo "Конфигурация: /tmp/${CLIENT_NAME}_amneziawg.conf"
echo ""
echo "Скачать конфиг:"
echo "docker cp amnezia-awg:/tmp/${CLIENT_NAME}_amneziawg.conf ./"
EOF

chmod +x /usr/local/bin/add_awg_user

# Использование:
add_awg_user alice 10.8.1.10

exit

# Скачать конфиг на локальную машину
docker cp amnezia-awg:/tmp/alice_amneziawg.conf ./
```

---

## Отзыв пользователя

```bash
# 1. Войти в контейнер
docker exec -it amnezia-awg bash

# 2. Найти и удалить секцию [Peer] пользователя
vi /opt/amnezia/awg/awg0.conf
# Удалить строки:
# [Peer]
# # alice
# PublicKey = ...
# PresharedKey = ...
# AllowedIPs = 10.8.1.10/32

# 3. Применить изменения (без перезапуска!)
awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)

# 4. Проверить
awg show
# Пользователя не должно быть в списке

exit
```

### Скрипт для удаления пользователя
```bash
docker exec -it amnezia-awg bash

cat > /usr/local/bin/remove_awg_user << 'EOF'
#!/bin/bash
PUBLIC_KEY=$1

if [ -z "$PUBLIC_KEY" ]; then
    echo "Использование: remove_awg_user <публичный_ключ>"
    exit 1
fi

# Создать временный файл без этого peer
awk -v key="$PUBLIC_KEY" '
BEGIN { skip=0 }
/^\[Peer\]/ { peer_section=1; buffer="[Peer]\n"; next }
peer_section && /^PublicKey/ { 
    if ($3 == key) { skip=1 } 
    else { print buffer $0; buffer="" }
    peer_section=0
    next
}
peer_section { buffer=buffer $0 "\n"; next }
skip && /^$/ { skip=0; next }
skip { next }
!skip { print }
' /opt/amnezia/awg/awg0.conf > /tmp/awg0.conf.new

mv /tmp/awg0.conf.new /opt/amnezia/awg/awg0.conf
awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)

echo "Пользователь с ключом $PUBLIC_KEY удален"
EOF

chmod +x /usr/local/bin/remove_awg_user

# Использование:
awg show  # Найти PublicKey пользователя
remove_awg_user "HIgo9xNzJMWLKASShiTqIybxZ0U3wGLiUeHV6U2v220="

exit
```

---

## Просмотр подключений

```bash
# Показать все подключения
docker exec amnezia-awg awg show

# Вывод:
# interface: awg0
#   public key: <серверный ключ>
#   listening port: 51820
#
# peer: HIgo9xNzJMWLKASShiTqIybxZ0U3wGLiUeHV6U2v220=
#   endpoint: 95.123.45.67:54321
#   allowed ips: 10.8.1.10/32
#   latest handshake: 2 minutes, 15 seconds ago
#   transfer: 150.5 MiB received, 45.2 MiB sent

# Показать только активные (handshake < 3 минуты)
docker exec amnezia-awg bash -c "awg show all dump | awk '\$6 < 180'"

# Подсчитать количество подключенных клиентов
docker exec amnezia-awg bash -c "awg show all dump | tail -n +2 | wc -l"
```

---

## Изменение параметров обфускации

### Базовые сценарии

#### Нет блокировок (максимальная скорость)
```bash
docker exec -it amnezia-awg bash
vi /opt/amnezia/awg/awg0.conf

# Изменить на:
Jc = 0
Jmin = 10
Jmax = 20
S1 = 0
S2 = 0
S3 = 0
S4 = 0
H1 = 0-1
H2 = 2-3
H3 = 4-5
H4 = 6-7

# Применить
awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)
exit
```

#### Средняя блокировка (рекомендуется)
```bash
docker exec -it amnezia-awg bash
vi /opt/amnezia/awg/awg0.conf

# Изменить на:
Jc = 4
Jmin = 15
Jmax = 50
S1 = 75
S2 = 120
S3 = 28
S4 = 10
H1 = 5-1234567
H2 = 1234568-5678901
H3 = 5678902-9876543
H4 = 9876544-2147483647

# Применить
awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)
exit
```

#### Максимальная обфускация
```bash
docker exec -it amnezia-awg bash
vi /opt/amnezia/awg/awg0.conf

# Изменить на:
Jc = 7
Jmin = 20
Jmax = 120
S1 = 145
S2 = 138
S3 = 55
S4 = 18
H1 = 10-5000000
H2 = 5000001-50000000
H3 = 50000001-500000000
H4 = 500000001-2147483647

# Применить
awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)
exit
```

### Скрипт генерации случайных параметров
```bash
docker exec -it amnezia-awg bash

cat > /usr/local/bin/randomize_awg_params << 'EOF'
#!/bin/bash

JC=$((RANDOM % 5 + 3))
JMIN=$((RANDOM % 20 + 10))
JMAX=$((RANDOM % 100 + 50))
S1=$((RANDOM % 135 + 15))
S2=$((RANDOM % 135 + 15))
while [ $S1 -eq $S2 ]; do S2=$((RANDOM % 135 + 15)); done
S3=$((RANDOM % 64))
S4=$((RANDOM % 20))

H1_MIN=$((RANDOM % 1000 + 1))
H1_MAX=$((RANDOM % 10000000 + H1_MIN))
H2_MIN=$((H1_MAX + 1))
H2_MAX=$((RANDOM % 50000000 + H2_MIN))
H3_MIN=$((H2_MAX + 1))
H3_MAX=$((RANDOM % 500000000 + H3_MIN))
H4_MIN=$((H3_MAX + 1))
H4_MAX=2147483647

sed -i "s/^Jc =.*/Jc = $JC/" /opt/amnezia/awg/awg0.conf
sed -i "s/^Jmin =.*/Jmin = $JMIN/" /opt/amnezia/awg/awg0.conf
sed -i "s/^Jmax =.*/Jmax = $JMAX/" /opt/amnezia/awg/awg0.conf
sed -i "s/^S1 =.*/S1 = $S1/" /opt/amnezia/awg/awg0.conf
sed -i "s/^S2 =.*/S2 = $S2/" /opt/amnezia/awg/awg0.conf
sed -i "s/^S3 =.*/S3 = $S3/" /opt/amnezia/awg/awg0.conf
sed -i "s/^S4 =.*/S4 = $S4/" /opt/amnezia/awg/awg0.conf
sed -i "s/^H1 =.*/H1 = $H1_MIN-$H1_MAX/" /opt/amnezia/awg/awg0.conf
sed -i "s/^H2 =.*/H2 = $H2_MIN-$H2_MAX/" /opt/amnezia/awg/awg0.conf
sed -i "s/^H3 =.*/H3 = $H3_MIN-$H3_MAX/" /opt/amnezia/awg/awg0.conf
sed -i "s/^H4 =.*/H4 = $H4_MIN-$H4_MAX/" /opt/amnezia/awg/awg0.conf

awg syncconf awg0 <(awg-quick strip /opt/amnezia/awg/awg0.conf)

echo "Новые параметры:"
grep -E "^(Jc|Jmin|Jmax|S1|S2|S3|S4|H1|H2|H3|H4)" /opt/amnezia/awg/awg0.conf
echo ""
echo "ВАЖНО: Обновите конфигурации всех клиентов!"
EOF

chmod +x /usr/local/bin/randomize_awg_params

# Использование:
randomize_awg_params

exit
```

**КРИТИЧНО**: После изменения параметров обновите ВСЕ конфигурации клиентов!

---

## Смена порта

### Смена на порт 443 (рекомендуется для обхода блокировок)

```bash
# 1. Остановить контейнер
docker stop amnezia-awg

# 2. Backup
docker cp amnezia-awg:/opt/amnezia/awg /tmp/awg-backup-$(date +%Y%m%d)

# 3. Удалить старый контейнер
docker rm amnezia-awg

# 4. Создать новый с портом 443
docker run -d \
  --privileged \
  --log-driver none \
  --restart always \
  --cap-add=NET_ADMIN \
  --cap-add=NET_RAW \
  --cap-add=SYS_MODULE \
  -v /lib/modules:/lib/modules \
  -p 443:51820/udp \
  --name amnezia-awg \
  --sysctl net.ipv4.conf.all.src_valid_mark=1 \
  amnezia-awg

# 5. Восстановить данные
docker cp /tmp/awg-backup-* amnezia-awg:/opt/amnezia/awg

# 6. Перезапустить
docker restart amnezia-awg

# 7. Проверить
docker exec amnezia-awg awg show
netstat -tuln | grep 443
```

**В конфигах клиентов** изменить:
```ini
[Peer]
Endpoint = ваш-ip:443  # Было :51820
```

---

## Полезные команды

```bash
# Просмотр конфигурации сервера
docker exec amnezia-awg cat /opt/amnezia/awg/awg0.conf

# Просмотр логов
docker logs --tail 100 amnezia-awg

# Статус контейнера
docker ps --filter name=amnezia-awg

# Использование ресурсов
docker stats amnezia-awg --no-stream

# Перезапуск контейнера
docker restart amnezia-awg

# Backup всей конфигурации
docker exec amnezia-awg tar -czf /tmp/awg-backup.tar.gz /opt/amnezia/awg/
docker cp amnezia-awg:/tmp/awg-backup.tar.gz ./awg-backup-$(date +%Y%m%d).tar.gz

# Восстановление из backup
docker cp awg-backup-20240125.tar.gz amnezia-awg:/tmp/
docker exec amnezia-awg tar -xzf /tmp/awg-backup-20240125.tar.gz -C /
docker restart amnezia-awg
```

---

## Диагностика проблем

```bash
# Проверка handshake
docker exec amnezia-awg awg show | grep "latest handshake"
# Если "Never" - проблема с подключением

# Проверка порта
netstat -tuln | grep 51820
# Должен показать: udp 0.0.0.0:51820

# Проверка firewall
iptables -L -n | grep 51820

# Тест связи с клиента
ping -c 4 10.8.1.1  # IP сервера в VPN

# Проверка DNS
nslookup google.com
```

---

**Подробные руководства:**
- `docs/AMNEZIAWG_MANUAL_MANAGEMENT.md` - Полное руководство по управлению
- `docs/AMNEZIAWG_ADVANCED_CONFIGURATION.md` - Детальное описание параметров и оптимизация
