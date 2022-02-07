set AmneziaPath=%~dp0
echo %AmneziaPath%

"%AmneziaPath%\AmneziaVPN.exe" -c
timeout /t 1
sc stop AmneziaVPN-service
sc delete AmneziaVPN-service
