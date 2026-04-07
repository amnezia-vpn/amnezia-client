@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SERVICE_NAME=FBLinkVPN-service"
set "SERVICE_EXE=%~dp0FBLinkVPN-service.exe"
set "SC=%SystemRoot%\System32\sc.exe"

if not exist "%SERVICE_EXE%" (
  echo ERROR: service executable not found: "%SERVICE_EXE%"
  exit /b 1
)

call "%~dp0cleanup_services.cmd" >nul 2>nul

call :upsert_service
if errorlevel 1 exit /b 1

"%SC%" failure "%SERVICE_NAME%" reset= 100 actions= restart/2000/restart/2000/restart/2000 >nul 2>nul

for /L %%I in (1,1,20) do (
  call :is_service_running
  if !errorlevel! EQU 0 exit /b 0
  "%SC%" start "%SERVICE_NAME%" >nul 2>nul
  if !errorlevel! EQU 1056 exit /b 0
  if !errorlevel! EQU 1058 (
    "%SC%" config "%SERVICE_NAME%" start= auto >nul 2>nul
    "%SC%" start "%SERVICE_NAME%" >nul 2>nul
    if !errorlevel! EQU 1056 exit /b 0
  )
  ping -n 2 127.0.0.1 >nul
)

echo ERROR: service "%SERVICE_NAME%" did not reach RUNNING state.
exit /b 1

:upsert_service
for /L %%I in (1,1,10) do (
  "%SC%" create "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" start= auto depend= BFE/nsi >nul 2>nul
  set "CREATE_RC=!errorlevel!"

  if "!CREATE_RC!"=="0" (
    call :config_service
    if !errorlevel! EQU 0 exit /b 0
  ) else if "!CREATE_RC!"=="1073" (
    call :config_service
    if !errorlevel! EQU 0 exit /b 0
  ) else if "!CREATE_RC!"=="1072" (
    ping -n 2 127.0.0.1 >nul
    "%SC%" delete "%SERVICE_NAME%" >nul 2>nul
    ping -n 2 127.0.0.1 >nul
  ) else (
    "%SC%" config "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" start= auto depend= BFE/nsi >nul 2>nul
    set "CONFIG_RC=!errorlevel!"
    if "!CONFIG_RC!"=="0" exit /b 0

    "%SC%" config "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" start= auto >nul 2>nul
    if !errorlevel! EQU 0 exit /b 0

    if "!CREATE_RC!"=="1639" (
      "%SC%" create "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" start= auto >nul 2>nul
      if !errorlevel! EQU 0 (
        call :config_service
        if !errorlevel! EQU 0 exit /b 0
      )
    )
  )
)

echo ERROR: failed to create/configure service "%SERVICE_NAME%".
exit /b 1

:config_service
"%SC%" config "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" start= auto depend= BFE/nsi >nul 2>nul
if !errorlevel! EQU 0 (
  rem Verify the path was actually written - sc config returns 0 even in some
  rem pending-delete states where the registry write is silently discarded.
  for /f "tokens=2 delims=: " %%A in ('"%SC%" qc "%SERVICE_NAME%" 2^>nul ^| findstr /i "BINARY_PATH_NAME"') do set "ACTUAL_EXE=%%A"
  echo "!ACTUAL_EXE!" | findstr /i /c:"%~dp0" >nul 2>nul
  if !errorlevel! EQU 0 exit /b 0
  rem Path mismatch - service is stale, force delete and let caller retry
  "%SC%" delete "%SERVICE_NAME%" >nul 2>nul
  exit /b 1
)

"%SC%" config "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" start= auto >nul 2>nul
if !errorlevel! EQU 0 exit /b 0

echo ERROR: failed to configure service "%SERVICE_NAME%".
exit /b 1

:is_service_running
powershell -NoProfile -ExecutionPolicy Bypass -Command "$s=Get-Service -Name '%SERVICE_NAME%' -ErrorAction SilentlyContinue; if ($null -ne $s -and $s.Status -eq 'Running') { exit 0 } else { exit 1 }" >nul 2>nul
exit /b %errorlevel%
