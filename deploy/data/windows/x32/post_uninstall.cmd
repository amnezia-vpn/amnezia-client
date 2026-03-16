set AmneziaPath=%~dp0
echo %AmneziaPath%

rem Define directories for logs
set "ORG_DIR=%AppData%\AmneziaVPN.ORG"
set "USER_APP_DIR=%ORG_DIR%\AmneziaVPN"
set "USER_LOG_DIR=%USER_APP_DIR%\log"
set "SYS_APP_DIR=%ProgramData%\AmneziaVPN"
set "SYS_LOG_DIR=%SYS_APP_DIR%\log"
set "SYS_LOG_FILE=%SYS_LOG_DIR%\AmneziaVPN-service.log"

timeout /t 1
sc stop FBLinkVPN-service 2>nul
sc delete FBLinkVPN-service 2>nul
rem Also clean up legacy AmneziaVPN service if present
sc stop AmneziaVPN-service 2>nul
sc delete AmneziaVPN-service 2>nul
sc stop AmneziaWGTunnel$AmneziaVPN 2>nul
sc delete AmneziaWGTunnel$AmneziaVPN 2>nul
sc stop AmneziaVPNSplitTunnel 2>nul
sc delete AmneziaVPNSplitTunnel 2>nul
taskkill /IM "FBLinkVPN-service.exe" /F 2>nul
taskkill /IM "FBLinkVPN.exe" /F 2>nul

rem Delete the service log file under ProgramData
if exist "%SYS_LOG_FILE%" del /F /Q "%SYS_LOG_FILE%"
if exist "%SYS_LOG_DIR%" rmdir /S /Q "%SYS_LOG_DIR%"
rem Try to remove application dir if empty
rd "%SYS_APP_DIR%" 2>nul

rem Delete client logs under current user's AppData\Roaming (Organization\Application)
if exist "%USER_LOG_DIR%" rmdir /S /Q "%USER_LOG_DIR%"
rem Try to remove app and org directories if empty
rd "%USER_APP_DIR%" 2>nul
rd "%ORG_DIR%" 2>nul

exit /b 0
