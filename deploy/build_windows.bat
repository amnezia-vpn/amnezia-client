@ECHO OFF

CHCP 1252

REM %VAR:"=% mean dequoted %VAR%

set PATH=%QT_BIN_DIR:"=%;%PATH%

echo "Using Qt in %QT_BIN_DIR%"
echo "Using QIF in %QIF_BIN_DIR%"
echo "Using WiX in %WIX_BIN_DIR%"

if "%WIX_BIN_DIR%"=="" (
    echo "WIX_BIN_DIR is not set"
    exit /b 1
)

set WIX_BIN_DIR_UNQUOTED=%WIX_BIN_DIR:"=%

set WIX_CLI=%WIX_BIN_DIR_UNQUOTED%\wix.exe

if not exist "%WIX_CLI%" (
    echo "WiX CLI (wix.exe) was not found in %WIX_BIN_DIR%"
    exit /b 1
)

REM Hold on to current directory
set PROJECT_DIR=%cd%
set SCRIPT_DIR=%PROJECT_DIR:"=%\deploy

set WORK_DIR=%SCRIPT_DIR:"=%\build_%BUILD_ARCH:"=%
set APP_NAME=AmneziaVPN
set APP_FILENAME=%APP_NAME:"=%.exe
set SERVICE_FILENAME=%APP_NAME:"=%-service.exe
set APP_DOMAIN=org.amneziavpn.package
set OUT_APP_DIR=%WORK_DIR:"=%\client\release
set PREBILT_DEPLOY_DATA_DIR=%PROJECT_DIR:"=%\client\3rd-prebuilt\deploy-prebuilt\windows\x%BUILD_ARCH:"=%
set DEPLOY_DATA_DIR=%SCRIPT_DIR:"=%\data\windows\x%BUILD_ARCH:"=%
set INSTALLER_DATA_DIR=%WORK_DIR:"=%\installer\packages\%APP_DOMAIN:"=%\data
set TARGET_FILENAME=%PROJECT_DIR:"=%\%APP_NAME:"=%_x%BUILD_ARCH:"=%.exe
set TARGET_MSI_FILENAME=%PROJECT_DIR:"=%\%APP_NAME:"=%_x%BUILD_ARCH:"=%.msi
set STAGE_DIR=%WORK_DIR:"=%\stage

echo "Environment:"
echo "WORK_DIR:             %WORK_DIR%"
echo "APP_FILENAME:         %APP_FILENAME%"
echo "SERVICE_FILENAME:     %SERVICE_FILENAME%"
echo "PROJECT_DIR:          %PROJECT_DIR%"
echo "SCRIPT_DIR:           %SCRIPT_DIR%"
echo "OUT_APP_DIR:          %OUT_APP_DIR%"
echo "DEPLOY_DATA_DIR:      %DEPLOY_DATA_DIR%"
echo "INSTALLER_DATA_DIR:   %INSTALLER_DATA_DIR%"
echo "TARGET_FILENAME:      %TARGET_FILENAME%"
echo "TARGET_MSI_FILENAME:  %TARGET_MSI_FILENAME%"
echo "STAGE_DIR:            %STAGE_DIR%"

echo "Cleanup..."
rmdir /Q /S %WORK_DIR%
del %TARGET_FILENAME%
del %TARGET_MSI_FILENAME%
rmdir /Q /S "%STAGE_DIR%"

mkdir %WORK_DIR%

call "%QT_BIN_DIR:"=%\qt-cmake" --version
"%QT_BIN_DIR:"=%\windeployqt" -v
cmake --version

cd %PROJECT_DIR%
call cmake . -B %WORK_DIR%  "-DCMAKE_BUILD_TYPE:STRING=Release" "-DCMAKE_PREFIX_PATH:PATH=%QT_BIN_DIR%"

cd %WORK_DIR%
cmake --build . --config release -- /p:UseMultiToolTask=true /m
if %errorlevel% neq 0 exit /b %errorlevel%

echo "Deploying..."

mkdir "%OUT_APP_DIR%"
copy "%WORK_DIR%\service\server\release\%SERVICE_FILENAME%" "%OUT_APP_DIR%"
rem copy "%WORK_DIR%\client\%APP_FILENAME%" "%OUT_APP_DIR%"

copy /Y "%PROJECT_DIR%\client\images\app.ico" "%OUT_APP_DIR%\AmneziaVPN.ico" >nul

echo "Signing exe"
cd %OUT_APP_DIR%
signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.exe

"%QT_BIN_DIR:"=%\windeployqt" --release --qmldir "%PROJECT_DIR:"=%\client"  --force --no-translations --force-openssl "%OUT_APP_DIR:"=%\%APP_FILENAME:"=%"
"%QT_BIN_DIR:"=%\windeployqt" --release "%OUT_APP_DIR:"=%\%SERVICE_FILENAME:"=%"

signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.dll

echo "Copying deploy data..."
xcopy %DEPLOY_DATA_DIR%    %OUT_APP_DIR%  /s /e /y /i /f
xcopy %PREBILT_DEPLOY_DATA_DIR%    %OUT_APP_DIR%  /s /e /y /i /f

cd %SCRIPT_DIR%
xcopy %SCRIPT_DIR:"=%\installer  %WORK_DIR:"=%\installer /s /e /y /i /f
mkdir %INSTALLER_DATA_DIR%

echo "Deploy finished, content:"
dir %OUT_APP_DIR%

cd %OUT_APP_DIR%
echo "Compressing data..."
"%QIF_BIN_DIR:"=%\archivegen" -c 9 %INSTALLER_DATA_DIR:"=%\%APP_NAME:"=%.7z .

cd "%WORK_DIR:"=%\installer"
echo "Creating installer..."
"%QIF_BIN_DIR:"=%\binarycreator" --offline-only -v -c config\windows.xml -p packages -f %TARGET_FILENAME%

timeout 5

cd %PROJECT_DIR%
signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 "%TARGET_FILENAME%"

echo "Preparing staging directory for MSI..."
rmdir /Q /S "%STAGE_DIR%"
mkdir "%STAGE_DIR%"
xcopy "%OUT_APP_DIR%" "%STAGE_DIR%" /s /e /y /i /f >nul

echo "Building MSI via CPack..."
rmdir /Q /S "%WORK_DIR%\_CPack_Packages"
cd %WORK_DIR%
cpack -G WIX -C Release --config "%WORK_DIR%\CPackConfig.cmake"
if exist "%WORK_DIR%\_CPack_Packages\win64\WIX\wix.log" (
    echo ---------------------------------------------
    echo Contents of wix.log:
    type "%WORK_DIR%\_CPack_Packages\win64\WIX\wix.log"
    echo ---------------------------------------------
)
if %errorlevel% neq 0 exit /b %errorlevel%

set GENERATED_MSI=
for /f "delims=" %%i in ('dir /b /a:-d /o:-d "%WORK_DIR%\*.msi"') do (
    if not defined GENERATED_MSI set GENERATED_MSI=%WORK_DIR%\%%i
)

if "%GENERATED_MSI%"=="" (
    echo "Failed to locate generated MSI package"
    exit /b 1
)

copy /Y "%GENERATED_MSI%" "%TARGET_MSI_FILENAME%"
if %errorlevel% neq 0 exit /b %errorlevel%

cd %PROJECT_DIR%
signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 "%TARGET_MSI_FILENAME%"

echo "Finished, see %TARGET_FILENAME% and %TARGET_MSI_FILENAME%"
exit 0
