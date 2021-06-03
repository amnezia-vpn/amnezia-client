 @ECHO OFF

CHCP 1252

REM %VAR:"=% mean dequoted %VAR%

set PATH=%QT_BIN_DIR:"=%;%PATH%

echo "Using Qt in %QT_BIN_DIR%"
echo "Using QIF in %QIF_BIN_DIR%"

REM Hold on to current directory
set PROJECT_DIR=%cd%
set SCRIPT_DIR=%PROJECT_DIR:"=%\deploy

set WORK_DIR=%SCRIPT_DIR:"=%\build_%BUILD_ARCH:"=%
rmdir /Q /S %WORK_DIR%
mkdir %WORK_DIR%


set APP_NAME=AmneziaVPN
set APP_FILENAME=%APP_NAME:"=%.exe
set APP_DOMAIN=org.amneziavpn.package
set RELEASE_DIR=%WORK_DIR:"=%
set OUT_APP_DIR=%RELEASE_DIR:"=%\client\release
set DEPLOY_DATA_DIR=%SCRIPT_DIR:"=%\data\windows\x%BUILD_ARCH:"=%
set INSTALLER_DATA_DIR=%RELEASE_DIR:"=%\installer\packages\%APP_DOMAIN:"=%\data
set PRO_FILE_PATH=%PROJECT_DIR:"=%\%APP_NAME:"=%.pro
set QMAKE_STASH_FILE=%PROJECT_DIR:"=%\.qmake_stash
set TARGET_FILENAME=%PROJECT_DIR:"=%\%APP_NAME:"=%_x%BUILD_ARCH:"=%.exe

echo "Environment:"
echo "APP_FILENAME:			%APP_FILENAME%"
echo "PROJECT_DIR:			%PROJECT_DIR%"
echo "SCRIPT_DIR:                   %SCRIPT_DIR%"
echo "RELEASE_DIR:			%RELEASE_DIR%"
echo "OUT_APP_DIR:			%OUT_APP_DIR%"
echo "DEPLOY_DATA_DIR:              %DEPLOY_DATA_DIR%"
echo "INSTALLER_DATA_DIR: 		%INSTALLER_DATA_DIR%"
echo "PRO_FILE_PATH:                %PRO_FILE_PATH%"
echo "QMAKE_STASH_FILE: 		%QMAKE_STASH_FILE%"
echo "TARGET_FILENAME:              %TARGET_FILENAME%"

rem Signing staff
powershell Set-ExecutionPolicy -ExecutionPolicy Unrestricted -Scope LocalMachine
powershell Get-ExecutionPolicy -List

powershell Import-PfxCertificate -FilePath %SCRIPT_DIR:"=%\PrivacyTechWindowsCert.pfx -CertStoreLocation Cert:\LocalMachine\My -Password $(ConvertTo-SecureString -String $Env:WIN_CERT_PW -AsPlainText -Force)

echo "Cleanup..."
Rmdir /Q /S %RELEASE_DIR%
Del %QMAKE_STASH_FILE%
Del %TARGET_FILENAME%

"%QT_BIN_DIR:"=%\qmake" -v
"%QT_BIN_DIR:"=%\windeployqt" -v
nmake /?

cd %PROJECT_DIR%
"%QT_BIN_DIR:"=%\qmake" -spec win32-msvc  -o "%WORK_DIR:"=%\Makefile"

cd %WORK_DIR%
set CL=/MP
nmake /A /NOLOGO
if %errorlevel% neq 0 exit /b %errorlevel%

nmake clean
rem if not exist "%OUT_APP_DIR:"=%\%APP_FILENAME:"=%" break

echo "Deploying..."

copy "%WORK_DIR:"=%\service\server\release\%APP_NAME:"=%-service.exe"	%OUT_APP_DIR%
copy "%WORK_DIR:"=%\platform\post-uninstall\release\post-uninstall.exe"	%OUT_APP_DIR%

echo "Signing exe"
cd %OUT_APP_DIR%
signtool sign /v /sm /s My /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.exe
 
"%QT_BIN_DIR:"=%\windeployqt" --release --force --no-translations "%OUT_APP_DIR:"=%\%APP_FILENAME:"=%"

signtool sign /v /sm /s My /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.dll

echo "Copying deploy data..."
xcopy %DEPLOY_DATA_DIR%    %OUT_APP_DIR%  /s /e /y /i /f


cd %SCRIPT_DIR%
xcopy %SCRIPT_DIR:"=%\installer  %RELEASE_DIR:"=%\installer /s /e /y /i /f
mkdir %INSTALLER_DATA_DIR%

echo "Deploy finished, content:"
dir %OUT_APP_DIR%

cd %OUT_APP_DIR%
echo "Compressing data..."
"%QIF_BIN_DIR:"=%\archivegen" -c 9 %INSTALLER_DATA_DIR:"=%\%APP_NAME:"=%.7z ./

cd "%RELEASE_DIR:"=%\installer"
echo "Creating installer..."
"%QIF_BIN_DIR:"=%\binarycreator" --offline-only -v -c config\windows.xml -p packages -f %TARGET_FILENAME%

timeout 5

cd %PROJECT_DIR%
signtool sign /v /sm /s My /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 "%TARGET_FILENAME%"

echo "Finished, see %TARGET_FILENAME%"
exit 0
