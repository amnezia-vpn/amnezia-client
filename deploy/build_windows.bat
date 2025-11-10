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
set APP_NAME=AmneziaVPN
set APP_FILENAME=%APP_NAME:"=%.exe
set APP_DOMAIN=org.amneziavpn.package
set OUT_APP_DIR=%WORK_DIR:"=%\client\Release
set PREBILT_DEPLOY_DATA_DIR=%PROJECT_DIR:"=%\client\3rd-prebuilt\deploy-prebuilt\windows\x%BUILD_ARCH:"=%
set DEPLOY_DATA_DIR=%SCRIPT_DIR:"=%\data\windows\x%BUILD_ARCH:"=%
set INSTALLER_DATA_DIR=%WORK_DIR:"=%\installer\packages\%APP_DOMAIN:"=%\data
set TARGET_FILENAME=%PROJECT_DIR:"=%\%APP_NAME:"=%_x%BUILD_ARCH:"=%.exe

echo "Environment:"
echo "WORK_DIR:             %WORK_DIR%"
echo "APP_FILENAME:         %APP_FILENAME%"
echo "PROJECT_DIR:          %PROJECT_DIR%"
echo "SCRIPT_DIR:           %SCRIPT_DIR%"
echo "OUT_APP_DIR:          %OUT_APP_DIR%"
echo "DEPLOY_DATA_DIR:      %DEPLOY_DATA_DIR%"
echo "INSTALLER_DATA_DIR:   %INSTALLER_DATA_DIR%"
echo "TARGET_FILENAME:      %TARGET_FILENAME%"

echo "Cleanup..."
rmdir /Q /S %WORK_DIR%
del %TARGET_FILENAME%

mkdir %WORK_DIR%

call "%QT_BIN_DIR:"=%\qt-cmake" --version
"%QT_BIN_DIR:"=%\windeployqt" -v
cmake --version

cd %PROJECT_DIR%
rem Derive QT_ROOT from QT_BIN_DIR (which points to the bin directory)
for %%I in ("%QT_BIN_DIR:"=%\..") do set QT_ROOT=%%~fI
call cmake . -B %WORK_DIR%  "-DCMAKE_BUILD_TYPE:STRING=Release" "-DCMAKE_PREFIX_PATH:PATH=%QT_ROOT%"

cd %WORK_DIR%
cmake --build . --config Release -- /p:UseMultiToolTask=true /m
if %errorlevel% neq 0 exit /b %errorlevel%

echo "Deploying..."

mkdir "%OUT_APP_DIR%"
if exist "%WORK_DIR%\service\server\Release\%APP_NAME%-service.exe" copy "%WORK_DIR%\service\server\Release\%APP_NAME%-service.exe" "%OUT_APP_DIR%"
if exist "%WORK_DIR%\client\Release\%APP_FILENAME%" (
  rem EXE already in OUT_APP_DIR; ensure target exists for windeployqt
) else if exist "%WORK_DIR%\client\%APP_FILENAME%" (
  copy "%WORK_DIR%\client\%APP_FILENAME%" "%OUT_APP_DIR%"
)

echo "Signing exe"
cd %OUT_APP_DIR%
if defined NO_SIGN (
  echo "NO_SIGN is set, skipping signing of EXE files"
) else (
  signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.exe
)

"%QT_BIN_DIR:"=%\windeployqt" --release --qmldir "%PROJECT_DIR:"=%\client" --force --no-translations --compiler-runtime "%OUT_APP_DIR:"=%\%APP_FILENAME:"=%"

if exist "%QT_ROOT%\qml\Qt\RemoteObjects\*.qml" (
    xcopy "%QT_ROOT%\qml\Qt\RemoteObjects" "%OUT_APP_DIR:"=%\qml\Qt\RemoteObjects" /s /e /y /i /f
)

rem Ensure Qt6RemoteObjects.dll is present
if not exist "%OUT_APP_DIR:"=%\Qt6RemoteObjects.dll" (
  echo "Qt6RemoteObjects.dll not found in OUT_APP_DIR, attempting to copy from Qt bin..."
  if exist "%QT_ROOT%\bin\Qt6RemoteObjects.dll" (
    copy "%QT_ROOT%\bin\Qt6RemoteObjects.dll" "%OUT_APP_DIR:"=%"
  ) else (
    echo "ERROR: %QT_ROOT%\bin\Qt6RemoteObjects.dll not found"
  )
) else (
  echo "Qt6RemoteObjects.dll already present in OUT_APP_DIR"
)

if not exist "%OUT_APP_DIR:"=%\Qt6RemoteObjects.dll" (
  echo "FATAL: Qt6RemoteObjects.dll is missing after copy attempts"
  dir "%OUT_APP_DIR:"=%"
  exit /b 1
)

if defined NO_SIGN (
  echo "NO_SIGN is set, skipping signing of DLL files"
) else (
  signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.dll
)

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
if defined NO_SIGN (
  echo "NO_SIGN is set, skipping signing of installer"
) else (
  signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 "%TARGET_FILENAME%"
)

echo "Finished, see %TARGET_FILENAME%"
exit 0
