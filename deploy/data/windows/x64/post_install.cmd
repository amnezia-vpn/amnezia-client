@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "SERVICE_NAME=FBLinkVPN-service"
set "SERVICE_EXE=%~dp0FBLinkVPN-service.exe"
set "LOG=%TEMP%\FBLinkVPN-installer.log"

echo [post_install] starting service verification >> "%LOG%" 2>&1

if exist "%~dp0install_service.cmd" (
  call "%~dp0install_service.cmd" >> "%LOG%" 2>&1
)

sc query "%SERVICE_NAME%" >nul 2>nul
if %errorlevel% EQU 0 goto :done

if not exist "%SERVICE_EXE%" (
  echo [post_install] ERROR: service executable not found at "%SERVICE_EXE%" >> "%LOG%" 2>&1
  exit /b 1
)

echo [post_install] fallback: creating service via PowerShell >> "%LOG%" 2>&1
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
  "$ErrorActionPreference='Stop'; $svc='%SERVICE_NAME%'; $exe='%SERVICE_EXE%'; $bin=('\"{0}\"' -f $exe); $s=Get-Service -Name $svc -ErrorAction SilentlyContinue; if($null -eq $s){ New-Service -Name $svc -BinaryPathName $bin -DisplayName 'FBLink VPN Service' -Description 'Service for FBLink VPN' -StartupType Automatic | Out-Null } else { sc.exe config $svc binPath= $bin start= auto | Out-Null }; sc.exe failure $svc reset= 100 actions= restart/2000/restart/2000/restart/2000 | Out-Null; try { Start-Service -Name $svc -ErrorAction SilentlyContinue } catch {};"

sc query "%SERVICE_NAME%" >nul 2>nul
if %errorlevel% EQU 0 goto :done

echo [post_install] ERROR: service is still missing after fallback >> "%LOG%" 2>&1
exit /b 1

:done
del /f /q "%Public%\Desktop\AmneziaVPN.lnk" >nul 2>nul
del /f /q "%ProgramData%\Microsoft\Windows\Start Menu\Programs\AmneziaVPN.lnk" >nul 2>nul
del /f /q "%AppData%\Microsoft\Windows\Start Menu\Programs\AmneziaVPN.lnk" >nul 2>nul
echo Installation complete. Please reboot your PC to apply VPN networking changes.
exit /b 0
