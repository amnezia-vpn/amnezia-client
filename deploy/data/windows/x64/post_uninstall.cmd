set AmneziaPath=%~dp0
echo %AmneziaPath%

rem Define directories for logs
set "ORG_DIR=%AppData%\AmneziaVPN.ORG"
set "USER_APP_DIR=%ORG_DIR%\AmneziaVPN"
set "USER_LOG_DIR=%USER_APP_DIR%\log"
set "SYS_APP_DIR=%ProgramData%\AmneziaVPN"
set "SYS_LOG_DIR=%SYS_APP_DIR%\log"
set "SYS_LOG_FILE=%SYS_LOG_DIR%\AmneziaVPN-service.log"

timeout /t 1 >nul
call :stop_and_delete_service "AmneziaVPN-service"
call :stop_and_delete_service "AmneziaWGTunnel$AmneziaVPN"
taskkill /IM "AmneziaVPN-service.exe" /F >nul 2>&1
taskkill /IM "AmneziaVPN.exe" /F >nul 2>&1
call :remove_split_tunnel_driver

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

:stop_and_delete_service
set "SVC_NAME=%~1"
sc query "%SVC_NAME%" >nul 2>&1 || goto :eof

sc stop "%SVC_NAME%" >nul 2>&1
for /l %%i in (1,1,15) do (
    sc query "%SVC_NAME%" | find "STOPPED" >nul 2>&1 && goto :delete_service
    timeout /t 1 /nobreak >nul
)

:delete_service
sc delete "%SVC_NAME%" >nul 2>&1
goto :eof

:remove_split_tunnel_driver
sc query "AmneziaVPNSplitTunnel" >nul 2>&1 || goto :eof
rem Explicit stop may trigger BSOD on some systems; delete safely instead.
sc config "AmneziaVPNSplitTunnel" start= disabled >nul 2>&1
sc delete "AmneziaVPNSplitTunnel" >nul 2>&1
goto :eof
