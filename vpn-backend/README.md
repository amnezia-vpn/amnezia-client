# VPN Backend

Go-бэкенд для коммерческого VPN-сервиса на базе AmneziaVPN.

## Быстрый старт

```bash
# 1. Скопируй .env и заполни
cp .env.example .env

# 2. Запусти
docker compose up -d

# 3. Проверь
curl http://localhost:8080/health
```

## API

### Auth
```
POST /api/v1/auth/register   {"email": "...", "password": "..."}
POST /api/v1/auth/login      {"email": "...", "password": "..."}
POST /api/v1/auth/refresh    {"refresh_token": "..."}
```

### Пользователь (требует Bearer token)
```
GET  /api/v1/me
GET  /api/v1/me/subscription
GET  /api/v1/me/config          ← скачать WireGuard .conf
POST /api/v1/me/config/revoke
```

### Платежи
```
POST /api/v1/payments/create    {"plan": "basic"|"premium"}
POST /api/v1/payments/webhook   ← ЮKassa webhook
```

### Admin (роль admin)
```
GET  /api/v1/admin/users
GET  /api/v1/admin/servers
POST /api/v1/admin/servers      {"name":"...", "host":"...", "public_key":"...", "region":"ru"}
GET  /api/v1/admin/payments
GET  /api/v1/admin/stats
```

## Добавление первого VPN-сервера

1. Установи AmneziaWG на VPS
2. Получи Public Key сервера (`wg genkey | tee priv | wg pubkey`)
3. Сделай запрос через admin:

```bash
curl -X POST https://yourvpn.app/api/v1/admin/servers \
  -H "Authorization: Bearer ADMIN_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"name":"RU-1","host":"1.2.3.4","public_key":"...","region":"ru","max_peers":100}'
```

## Деплой на сервер

```bash
# Установи TLS
apt install certbot
certbot certonly --standalone -d yourvpn.app

# Запусти
docker compose up -d
```

## Создание admin-пользователя

```bash
# Зарегистрируйся обычным способом, затем в SQLite:
docker exec -it vpn-backend sqlite3 /app/data/vpn.db \
  "UPDATE users SET role='admin' WHERE email='your@email.com';"
```
