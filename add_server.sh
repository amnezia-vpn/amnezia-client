#!/bin/bash

# Создать новую БД с пост-квантовой криптографией
echo "Добавление сервера в БД (VLESS + Post-Quantum)..."

curl -X POST http://31.135.65.188:8080/admin/add-server \
  -H "Content-Type: application/json" \
  -d @server.json

echo
echo "Готово!"
