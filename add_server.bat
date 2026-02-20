@echo off
chcp 65001 >nul

echo Добавление сервера с пост-квантовой криптографией...

curl.exe -X POST http://31.135.65.188:8080/admin/add-server ^
  -H "Content-Type: application/json" ^
  -d @server.json

echo.
echo Готово!
pause
