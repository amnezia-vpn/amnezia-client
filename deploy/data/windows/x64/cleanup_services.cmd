@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SERVICE_LIST=FBLinkVPN-service FBLink-service AmneziaVPN-service AmneziaWGTunnel$AmneziaVPN AmneziaWGTunnel$FBLink AmneziaVPNSplitTunnel FBLinkSplitTunnel"
set "PROCESS_LIST=FBLinkVPN-service.exe FBLink-service.exe AmneziaVPN-service.exe FBLinkVPN.exe FBLink.exe AmneziaVPN.exe"
set "FAILED=0"

for %%P in (%PROCESS_LIST%) do (
  taskkill /IM "%%P" /F >nul 2>nul
)

for %%S in (%SERVICE_LIST%) do (
  call :remove_service "%%~S"
  if errorlevel 1 set "FAILED=1"
)

if "%FAILED%"=="1" (
  echo ERROR: one or more legacy services could not be removed.
  exit /b 1
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
  timeout /t 1 /nobreak >nul
  sc stop "%SVC%" >nul 2>nul
  sc delete "%SVC%" >nul 2>nul
)

sc query "%SVC%" >nul 2>nul
set "RC=!errorlevel!"
if "!RC!"=="1060" (
  exit /b 0
)

echo ERROR: failed to remove service "%SVC%".
exit /b 1
