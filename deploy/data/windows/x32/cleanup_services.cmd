@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "CRITICAL_SERVICE_LIST=FBLinkVPN-service FBLink-service AmneziaVPN-service"
set "LEGACY_SERVICE_LIST=AmneziaWGTunnel$AmneziaVPN AmneziaWGTunnel$FBLink AmneziaVPNSplitTunnel FBLinkSplitTunnel"
set "PROCESS_LIST=FBLinkVPN-service.exe FBLink-VPN-service.exe FBLink-service.exe AmneziaVPN-service.exe FBLinkVPN.exe FBLink.exe AmneziaVPN.exe"
set "FAILED=0"

for %%P in (%PROCESS_LIST%) do (
  taskkill /IM "%%P" /F >nul 2>nul
)

for %%S in (%CRITICAL_SERVICE_LIST%) do (
  call :remove_service "%%~S"
  if errorlevel 1 set "FAILED=1"
)

for %%S in (%LEGACY_SERVICE_LIST%) do (
  call :remove_service "%%~S"
  if errorlevel 1 (
    echo WARNING: failed to remove optional legacy service "%%~S".
  )
)

if "%FAILED%"=="1" (
  echo WARNING: one or more critical services could not be removed. Installation will continue and service configuration will be repaired.
  exit /b 0
)

exit /b 0

:remove_service
set "SVC=%~1"

sc stop "%SVC%" >nul 2>nul
sc delete "%SVC%" >nul 2>nul

for /L %%I in (1,1,30) do (
  sc query "%SVC%" >nul 2>nul
  set "RC=!errorlevel!"
  if "!RC!"=="1060" exit /b 0
  ping -n 2 127.0.0.1 >nul
  sc stop "%SVC%" >nul 2>nul
  sc delete "%SVC%" >nul 2>nul
)

sc query "%SVC%" >nul 2>nul
set "RC=!errorlevel!"
if "!RC!"=="1060" (
  exit /b 0
)

echo WARN: failed to fully remove service "%SVC%".
exit /b 1
