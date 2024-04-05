sc stop WireGuardTunnel$AmneziaVPN
sc delete WireGuardTunnel$AmneziaVPN
taskkill /IM "AmneziaVPN-service.exe" /F
exit /b 0
