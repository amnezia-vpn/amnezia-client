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

echo "Checking MSVC environment..."
where cl >nul 2>&1
if %errorlevel% equ 0 (
    echo "MSVC compiler found"
    cl 2>&1 | findstr /C:"Version"
) else (
    echo "ERROR: MSVC compiler not found in PATH"
    echo "Please ensure 'Setup mvsc' step in GitHub Actions workflow is configured correctly"
    echo "Trying to find cl.exe..."
    where cl 2>&1
    exit /b 1
)

call "%QT_BIN_DIR:"=%\qt-cmake" --version
if defined QT_HOST_BIN_DIR (
    "%QT_HOST_BIN_DIR:"=%\windeployqt" -v
) else (
    "%QT_BIN_DIR:"=%\windeployqt" -v
)
cmake --version

REM Calculate Qt root directory BEFORE entering if blocks (batch variable expansion issue)
REM Remove \bin from QT_BIN_DIR to get Qt root
set "QT_ROOT_DIR=%QT_BIN_DIR:\bin=%"

cd %PROJECT_DIR%
if defined QT_HOST_PATH (
    REM ARM64 cross-compilation - set generator platform to ARM64
    REM CMAKE_PREFIX_PATH must point to Qt root (not bin), so CMake can find lib directory
    
    echo "=== CMake Configuration for ARM64 ==="
    echo "QT_BIN_DIR:              %QT_BIN_DIR%"
    echo "QT_ROOT_DIR:             %QT_ROOT_DIR%"
    echo "QT_HOST_PATH:            %QT_HOST_PATH%"
    echo "CMAKE_PREFIX_PATH:       %QT_ROOT_DIR%"
    echo "Generator Platform:      ARM64"
    echo "===================================="
    
    REM Verify Qt ARM64 directories exist
    if not exist "%QT_ROOT_DIR%" (
        echo "ERROR: Qt ARM64 root directory not found at %QT_ROOT_DIR%"
        exit /b 1
    )
    echo "Qt ARM64 root directory exists: %QT_ROOT_DIR%"
    
    if not exist "%QT_ROOT_DIR%\lib" (
        echo "ERROR: Qt ARM64 lib directory not found at %QT_ROOT_DIR%\lib"
        exit /b 1
    )
    echo "Qt ARM64 lib directory found: %QT_ROOT_DIR%\lib"
    dir "%QT_ROOT_DIR%\lib\Qt6Core*.lib"
    
    REM Check architecture of Qt6Core.lib (if dumpbin is available)
    echo "Verifying Qt6Core.lib architecture..."
    where dumpbin >nul 2>&1
    if %errorlevel% equ 0 (
        echo "dumpbin found, checking architecture..."
        dumpbin /headers "%QT_ROOT_DIR%\lib\Qt6Core.lib" | findstr /C:"machine"
        if %errorlevel% equ 0 (
            echo "Architecture check completed"
        ) else (
            echo "WARNING: Could not extract machine type from Qt6Core.lib"
        )
    ) else (
        echo "NOTE: dumpbin not available, skipping architecture verification"
        echo "This is not critical - build will continue"
    )
    
    call cmake . -B %WORK_DIR% -A ARM64 "-DCMAKE_BUILD_TYPE:STRING=Release" "-DCMAKE_PREFIX_PATH:PATH=%QT_ROOT_DIR%" "-DQT_HOST_PATH:PATH=%QT_HOST_PATH%"
) else (
    REM x64 build - QT_ROOT_DIR already calculated above
    echo "=== CMake Configuration for x64 ==="
    echo "QT_ROOT_DIR: %QT_ROOT_DIR%"
    echo "===================================="
    
    call cmake . -B %WORK_DIR%  "-DCMAKE_BUILD_TYPE:STRING=Release" "-DCMAKE_PREFIX_PATH:PATH=%QT_ROOT_DIR%"
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
    REM For ARM64 cross-compilation, manually copy all Qt ARM64 libraries
    REM windeployqt from x64 host copies x64 libraries, not ARM64
    echo "Manually copying ARM64 Qt libraries (windeployqt copies x64 instead)"
    echo "ARM64 Qt path: %QT_BIN_DIR:"=%"
    
    REM First, use windeployqt ONLY to detect QML dependencies (--dry-run would be ideal but not supported)
    REM We'll let it run to create directory structure, then overwrite with ARM64 DLLs
    echo "Running windeployqt to detect QML dependencies..."
    "%QT_HOST_BIN_DIR:"=%\windeployqt" --release --qmldir "%PROJECT_DIR:"=%\client" --force --no-translations --compiler-runtime "%OUT_APP_DIR:"=%\%APP_FILENAME:"=%"
    
    REM Now overwrite all x64 DLLs with ARM64 DLLs
    echo "Overwriting x64 Qt DLLs with ARM64 versions..."
    if exist "%QT_BIN_DIR:"=%\Qt6Core.dll" (
        echo "Copying all ARM64 Qt DLLs from %QT_BIN_DIR:"=%"
        copy "%QT_BIN_DIR:"=%\*.dll" "%OUT_APP_DIR:"=%\" /y >nul 2>&1
        
        REM Also copy DLLs from Qt root directory (d3dcompiler_47.dll, dxcompiler.dll, etc.)
        set "QT_ROOT=%QT_BIN_DIR:"=%\.."
        if exist "%QT_ROOT%\*.dll" (
            echo "Copying additional ARM64 DLLs from Qt root"
            copy "%QT_ROOT%\*.dll" "%OUT_APP_DIR:"=%\" /y >nul 2>&1
        )
        
        REM Copy all ARM64 Qt plugins (overwrite x64 versions)
        if exist "%QT_BIN_DIR:"=%\..\plugins" (
            echo "Copying ARM64 Qt plugins"
            xcopy "%QT_BIN_DIR:"=%\..\plugins" "%OUT_APP_DIR:"=%\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy ARM64 QML modules  
        if exist "%QT_BIN_DIR:"=%\..\qml" (
            echo "Copying ARM64 QML modules"
            xcopy "%QT_BIN_DIR:"=%\..\qml" "%OUT_APP_DIR:"=%\qml\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy translations
        if exist "%QT_BIN_DIR:"=%\..\translations" (
            echo "Copying translations"
            xcopy "%QT_BIN_DIR:"=%\..\translations" "%OUT_APP_DIR:"=%\translations\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy resources
        if exist "%QT_BIN_DIR:"=%\..\resources" (
            echo "Copying resources"
            xcopy "%QT_BIN_DIR:"=%\..\resources" "%OUT_APP_DIR:"=%\resources\" /s /e /y /i >nul 2>&1
        )
        
        REM Copy all additional Qt module directories that exist
        echo "Copying additional ARM64 Qt module directories..."
        set "QT_ROOT=%QT_BIN_DIR:"=%\.."
        
        REM List of Qt module directories to copy
        for %%d in (cloak cygwin generic iconengines imageformats networkinformation openvpn platforms qmltooling ss styles tap tls xray) do (
            if exist "%QT_ROOT%\%%d" (
                echo "Copying %%d directory"
                xcopy "%QT_ROOT%\%%d" "%OUT_APP_DIR:"=%\%%d\" /s /e /y /i >nul 2>&1
            )
        )
        
        echo "ARM64 Qt libraries copied successfully"
        
        REM Verify Qt6Core.dll architecture
        echo "Verifying Qt6Core.dll was copied:"
        if exist "%OUT_APP_DIR:"=%\Qt6Core.dll" (
            echo "Qt6Core.dll found in output directory"
        ) else (
            echo "ERROR: Qt6Core.dll not found after copy"
            exit /b 1
        )
    ) else (
        echo "ERROR: ARM64 Qt DLLs not found in %QT_BIN_DIR:"=%"
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
