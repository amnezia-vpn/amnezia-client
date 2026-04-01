rem Stop and remove old AmneziaVPN service (migration)
sc stop AmneziaVPN-service 2>nul
sc delete AmneziaVPN-service 2>nul
sc stop AmneziaWGTunnel$AmneziaVPN 2>nul
sc delete AmneziaWGTunnel$AmneziaVPN 2>nul

rem Keep current FBLink service registration intact.
rem The installer handles FBLinkVPN-service lifecycle in componentscript.js.
sc stop FBLinkVPN-service 2>nul
taskkill /IM "AmneziaVPN-service.exe" /F 2>nul
taskkill /IM "AmneziaVPN.exe" /F 2>nul
taskkill /IM "FBLinkVPN-service.exe" /F 2>nul
taskkill /IM "FBLinkVPN.exe" /F 2>nul
exit /b 0
