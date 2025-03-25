sc stop AmneziaWGTunnel$DefaultVPN
sc delete AmneziaWGTunnel$DefaultVPN
taskkill /IM "DefaultVPN-service.exe" /F
taskkill /IM "DefaultVPN.exe" /F
exit /b 0
