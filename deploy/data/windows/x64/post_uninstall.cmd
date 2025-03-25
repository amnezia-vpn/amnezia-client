set AmneziaPath=%~dp0
echo %AmneziaPath%

"%AmneziaPath%\DefaultVPN.exe" -c
timeout /t 1
sc stop DefaultVPN-service
sc delete DefaultVPN-service
sc stop AmneziaWGTunnel$DefaultVPN
sc delete AmneziaWGTunnel$DefaultVPN
taskkill /IM "DefaultVPN-service.exe" /F
taskkill /IM "DefaultVPN.exe" /F
exit /b 0
