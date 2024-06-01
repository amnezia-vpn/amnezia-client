sc stop WireGuardTunnel$VPNNaruzhu
sc delete WireGuardTunnel$VPNNaruzhu
taskkill /IM "VPNNaruzhu-service.exe" /F
taskkill /IM "VPNNaruzhu.exe" /F
exit /b 0
