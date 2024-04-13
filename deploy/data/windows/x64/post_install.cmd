sc stop WireGuardTunnel$AmneziaVPN
sc delete WireGuardTunnel$AmneziaVPN
taskkill /IM "AmneziaVPN-service.exe" /F
taskkill /IM "AmneziaVPN.exe" /F
exit /b 0
