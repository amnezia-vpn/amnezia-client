@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "INSTALL_DIR=%~dp0"

rem ── Stop and delete VPN services ─────────────────────────────────────────────
call "%INSTALL_DIR%cleanup_services.cmd"

rem ── Remove service log files ──────────────────────────────────────────────────
set "SYS_LOG_DIR=%ProgramData%\FBLinkVPN\log"
if exist "%SYS_LOG_DIR%" rmdir /S /Q "%SYS_LOG_DIR%"
rem Try to remove FBLinkVPN data dir if empty
rd "%ProgramData%\FBLinkVPN" 2>nul

rem ── Remove client logs under user AppData ────────────────────────────────────
set "USER_APP_DIR=%AppData%\FBLinkVPN.ORG\FBLinkVPN"
set "USER_LOG_DIR=%USER_APP_DIR%\log"
if exist "%USER_LOG_DIR%" rmdir /S /Q "%USER_LOG_DIR%"
rd "%USER_APP_DIR%" 2>nul
rd "%AppData%\FBLinkVPN.ORG" 2>nul

exit /b 0
