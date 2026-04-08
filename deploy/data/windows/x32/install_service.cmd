@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SERVICE_NAME=FBLinkVPN-service"
set "SERVICE_EXE=%~dp0FBLinkVPN-service.exe"
set "SC=%SystemRoot%\System32\sc.exe"

if not exist "%SERVICE_EXE%" (
  echo ERROR: service executable not found: "%SERVICE_EXE%"
  exit /b 1
)

rem -- Step 1: remove any stale service registration ----------------------------
call "%~dp0cleanup_services.cmd"

rem Wait up to 60 s for the service to be fully gone from SCM.
rem sc query returns 1060 = ERROR_SERVICE_DOES_NOT_EXIST when truly removed.
set "WAIT_I=0"
:wait_gone
"%SC%" query "%SERVICE_NAME%" >nul 2>nul
if !errorlevel! EQU 1060 goto :create_service
set /a WAIT_I+=1
if !WAIT_I! GEQ 30 (
  echo WARN: old service registry entry still present after 60 s - forcing removal
  "%SC%" stop   "%SERVICE_NAME%" >nul 2>nul
  "%SC%" delete "%SERVICE_NAME%" >nul 2>nul
  ping -n 4 127.0.0.1 >nul
  goto :create_service
)
ping -n 3 127.0.0.1 >nul
"%SC%" stop   "%SERVICE_NAME%" >nul 2>nul
"%SC%" delete "%SERVICE_NAME%" >nul 2>nul
goto :wait_gone

rem -- Step 2: register the service fresh ---------------------------------------
:create_service
echo Registering %SERVICE_NAME% from "%SERVICE_EXE%"
"%SC%" create "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" start= auto
set "CREATE_RC=!errorlevel!"
if !CREATE_RC! EQU 0 goto :configure_service
if !CREATE_RC! EQU 1073 (
  rem Lost the race - another process created it between our delete and create.
  rem Reconfigure to ensure correct binary path.
  echo WARN: service already exists - reconfiguring
  "%SC%" config "%SERVICE_NAME%" binPath= "\"%SERVICE_EXE%\"" start= auto
  if !errorlevel! EQU 0 goto :configure_service
)
echo ERROR: sc create failed with !CREATE_RC!
exit /b 1

rem -- Step 3: set display name and recovery options ----------------------------
:configure_service
"%SC%" config  "%SERVICE_NAME%" DisplayName= "FBLink VPN Service" >nul 2>nul
"%SC%" failure "%SERVICE_NAME%" reset= 100 actions= restart/2000/restart/2000/restart/2000 >nul 2>nul

rem -- Step 4: start the service and wait for RUNNING state ---------------------
for /L %%I in (1,1,20) do (
  call :is_service_running
  if !errorlevel! EQU 0 (
    echo Service %SERVICE_NAME% is RUNNING.
    exit /b 0
  )
  echo Attempt %%I: starting %SERVICE_NAME%...
  "%SC%" start "%SERVICE_NAME%"
  set "START_RC=!errorlevel!"
  if !START_RC! EQU 1056 (
    echo Service %SERVICE_NAME% is already running.
    exit /b 0
  )
  if !START_RC! EQU 1058 (
    rem SERVICE_DISABLED - re-enable and retry
    "%SC%" config "%SERVICE_NAME%" start= auto >nul 2>nul
  )
  ping -n 2 127.0.0.1 >nul
)

echo ERROR: service "%SERVICE_NAME%" did not reach RUNNING state after 40 s.
call :print_service_diagnostics
exit /b 1

:is_service_running
powershell -NoProfile -ExecutionPolicy Bypass -Command "$s=Get-Service -Name '%SERVICE_NAME%' -ErrorAction SilentlyContinue; if ($null -ne $s -and $s.Status -eq 'Running') { exit 0 } else { exit 1 }" >nul 2>nul
exit /b %errorlevel%

:print_service_diagnostics
echo ---- %SERVICE_NAME% diagnostics ----
"%SC%" query "%SERVICE_NAME%"
"%SC%" queryex "%SERVICE_NAME%"
"%SC%" qc "%SERVICE_NAME%"
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$svc = Get-CimInstance Win32_Service -Filter \"Name='%SERVICE_NAME%'\" -ErrorAction SilentlyContinue; if ($null -eq $svc) { Write-Host 'Service object: not found'; exit 0 }; Write-Host ('PathName=' + $svc.PathName); Write-Host ('State=' + $svc.State); Write-Host ('Status=' + $svc.Status); Write-Host ('StartMode=' + $svc.StartMode); Write-Host ('ExitCode=' + $svc.ExitCode); Write-Host ('ProcessId=' + $svc.ProcessId)"
echo ---- end diagnostics ----
exit /b 0
