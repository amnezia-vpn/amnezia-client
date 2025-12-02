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
set OUT_APP_DIR=%WORK_DIR:"=%\client\release

REM Determine architecture prefix for paths
if "%BUILD_ARCH%"=="arm64" (
    set ARCH_PREFIX=arm64
) else (
    set ARCH_PREFIX=x%BUILD_ARCH:"=%
)

set PREBILT_DEPLOY_DATA_DIR=%PROJECT_DIR:"=%\client\3rd-prebuilt\deploy-prebuilt\windows\%ARCH_PREFIX:"=%
set DEPLOY_DATA_DIR=%SCRIPT_DIR:"=%\data\windows\%ARCH_PREFIX:"=%
set INSTALLER_DATA_DIR=%WORK_DIR:"=%\installer\packages\%APP_DOMAIN:"=%\data
set TARGET_FILENAME=%PROJECT_DIR:"=%\%APP_NAME:"=%_x%BUILD_ARCH:"=%.exe

echo "Environment:"
echo "BUILD_ARCH:           %BUILD_ARCH%"
echo "ARCH_PREFIX:          %ARCH_PREFIX%"
if defined QT_HOST_PATH echo "QT_HOST_PATH:        %QT_HOST_PATH%"
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
if defined QT_HOST_BIN_DIR (
    "%QT_HOST_BIN_DIR:"=%\windeployqt" -v
) else (
    "%QT_BIN_DIR:"=%\windeployqt" -v
)
cmake --version

cd %PROJECT_DIR%
if defined QT_HOST_PATH (
    call cmake . -B %WORK_DIR%  "-DCMAKE_BUILD_TYPE:STRING=Release" "-DCMAKE_PREFIX_PATH:PATH=%QT_BIN_DIR%" "-DQT_HOST_PATH:PATH=%QT_HOST_PATH%"
) else (
    call cmake . -B %WORK_DIR%  "-DCMAKE_BUILD_TYPE:STRING=Release" "-DCMAKE_PREFIX_PATH:PATH=%QT_BIN_DIR%"
)

cd %WORK_DIR%
cmake --build . --config release -- /p:UseMultiToolTask=true /m
if %errorlevel% neq 0 exit /b %errorlevel%

echo "Deploying..."

mkdir "%OUT_APP_DIR%"
copy "%WORK_DIR%\service\server\release\%APP_NAME%-service.exe" "%OUT_APP_DIR%"
copy "%WORK_DIR%\client\Release\%APP_FILENAME%" "%OUT_APP_DIR%"


echo "Signing exe"
cd %OUT_APP_DIR%
REM For ARM64 cross-compilation, use x64 signtool from host (ARM64 signtool won't run on x64 Windows)
if defined QT_HOST_BIN_DIR (
    REM Find x64 signtool - look for x64 version in Windows Kits (skip arm64)
    set SIGNGTOOL_PATH=
    for /f "delims=" %%i in ('dir /b /s "C:\Program Files (x86)\Windows Kits\10\bin\*\x64\signtool.exe" 2^>nul') do (
        set SIGNGTOOL_PATH=%%i
        goto :found_signtool
    )
    :found_signtool
    if defined SIGNGTOOL_PATH (
        echo "Using x64 signtool: %SIGNGTOOL_PATH%"
        "%SIGNGTOOL_PATH%" sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.exe
    ) else (
        echo "Warning: Could not find x64 signtool, trying default signtool"
        signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.exe 2>nul || echo "Warning: Signing failed, continuing..."
    )
) else (
    signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.exe
)

REM For cross-compiled ARM64, use windeployqt from host Qt (x64) but point to ARM64 Qt libraries
if defined QT_HOST_BIN_DIR (
    echo "Using host windeployqt for cross-compilation (to copy ARM64 libraries)"
    echo "ARM64 exe file: %OUT_APP_DIR%\%APP_FILENAME%"
    echo "ARM64 Qt path: %QT_BIN_DIR%"
    REM Verify that ARM64 exe exists before running windeployqt
    if not exist "%OUT_APP_DIR%\%APP_FILENAME%" (
        echo "ERROR: ARM64 exe file not found: %OUT_APP_DIR%\%APP_FILENAME%"
        exit /b 1
    )
    REM For ARM64, manually copy Qt DLLs instead of using windeployqt
    REM windeployqt has issues with cross-compilation paths
    echo "Manually copying ARM64 Qt DLLs instead of using windeployqt"
    
    REM Debug: Check if Qt bin directory exists
    echo "Checking Qt bin directory: %QT_BIN_DIR:"=%"
    if exist "%QT_BIN_DIR:"=%" (
        echo "Qt bin directory exists"
        dir "%QT_BIN_DIR:"=%\Qt6*.dll" 2>nul
    ) else (
        echo "ERROR: Qt bin directory does not exist: %QT_BIN_DIR:"=%"
        exit /b 1
    )
    
    REM Copy Qt DLLs
    if exist "%QT_BIN_DIR:"=%\Qt6Core.dll" (
        echo "Copying all Qt6 DLLs from %QT_BIN_DIR:"=%"
        copy "%QT_BIN_DIR:"=%\*.dll" "%OUT_APP_DIR:"=%\" >nul 2>&1
        
        REM Copy Qt6 plugins
        if exist "%QT_BIN_DIR:"=%\..\plugins" (
            echo "Copying Qt6 plugins"
            xcopy "%QT_BIN_DIR:"=%\..\plugins" "%OUT_APP_DIR:"=%\plugins\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy Qt6 QML modules
        if exist "%QT_BIN_DIR:"=%\..\qml" (
            echo "Copying Qt6 QML modules"
            xcopy "%QT_BIN_DIR:"=%\..\qml" "%OUT_APP_DIR:"=%\qml\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy translations if needed
        if exist "%QT_BIN_DIR:"=%\..\translations" (
            echo "Copying Qt6 translations"
            xcopy "%QT_BIN_DIR:"=%\..\translations\qt*.qm" "%OUT_APP_DIR:"=%\translations\" /y /i >nul 2>&1
        )
        
        REM Copy resources directories
        if exist "%QT_BIN_DIR:"=%\..\resources" (
            echo "Copying Qt6 resources"
            xcopy "%QT_BIN_DIR:"=%\..\resources" "%OUT_APP_DIR:"=%\resources\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy iconengines
        if exist "%QT_BIN_DIR:"=%\..\plugins\iconengines" (
            echo "Copying iconengines"
            xcopy "%QT_BIN_DIR:"=%\..\plugins\iconengines" "%OUT_APP_DIR:"=%\iconengines\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy imageformats
        if exist "%QT_BIN_DIR:"=%\..\plugins\imageformats" (
            echo "Copying imageformats"
            xcopy "%QT_BIN_DIR:"=%\..\plugins\imageformats" "%OUT_APP_DIR:"=%\imageformats\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy platforms
        if exist "%QT_BIN_DIR:"=%\..\plugins\platforms" (
            echo "Copying platforms"
            xcopy "%QT_BIN_DIR:"=%\..\plugins\platforms" "%OUT_APP_DIR:"=%\platforms\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy styles
        if exist "%QT_BIN_DIR:"=%\..\plugins\styles" (
            echo "Copying styles"
            xcopy "%QT_BIN_DIR:"=%\..\plugins\styles" "%OUT_APP_DIR:"=%\styles\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy tls
        if exist "%QT_BIN_DIR:"=%\..\plugins\tls" (
            echo "Copying tls"
            xcopy "%QT_BIN_DIR:"=%\..\plugins\tls" "%OUT_APP_DIR:"=%\tls\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy networkinformation
        if exist "%QT_BIN_DIR:"=%\..\plugins\networkinformation" (
            echo "Copying networkinformation"
            xcopy "%QT_BIN_DIR:"=%\..\plugins\networkinformation" "%OUT_APP_DIR:"=%\networkinformation\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy generic
        if exist "%QT_BIN_DIR:"=%\..\plugins\generic" (
            echo "Copying generic"
            xcopy "%QT_BIN_DIR:"=%\..\plugins\generic" "%OUT_APP_DIR:"=%\generic\" /s /e /y /i >nul 2>&1
        )
        
        echo "Successfully copied Qt ARM64 libraries and resources"
    ) else (
        echo "ERROR: Qt DLLs not found in %QT_BIN_DIR:"=%"
        exit /b 1
    )
) else (
    "%QT_BIN_DIR:"=%\windeployqt" --release --qmldir "%PROJECT_DIR:"=%\client"  --force --no-translations "%OUT_APP_DIR:"=%\%APP_FILENAME:"=%"
)

REM Sign DLLs - use x64 signtool for ARM64 cross-compilation
cd %OUT_APP_DIR%
if defined QT_HOST_BIN_DIR (
    if defined SIGNGTOOL_PATH (
        "%SIGNGTOOL_PATH%" sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.dll
    ) else (
        echo "Warning: Could not find x64 signtool, trying default signtool for DLLs"
        signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 *.dll 2>nul || echo "Warning: DLL signing failed, continuing..."
    )
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
REM For ARM64 cross-compilation, use x64 signtool for final installer
if defined QT_HOST_BIN_DIR (
    if defined SIGNGTOOL_PATH (
        "%SIGNGTOOL_PATH%" sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 "%TARGET_FILENAME%"
    ) else (
        echo "Warning: Could not find x64 signtool, trying default signtool for installer"
        signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 "%TARGET_FILENAME%" 2>nul || echo "Warning: Installer signing failed, continuing..."
    )
) else (
    signtool sign /v /n "Privacy Technologies OU" /fd sha256 /tr http://timestamp.comodoca.com/?td=sha256 /td sha256 "%TARGET_FILENAME%"
)

echo "Finished, see %TARGET_FILENAME%"
exit 0
