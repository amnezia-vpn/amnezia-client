@echo off
setlocal EnableDelayedExpansion

set "PROJECT_DIR=%cd%"
set "BUILD_DIR=%PROJECT_DIR%\deploy\build"

:parse_args
if "%~1"=="" goto :done_args
if /i "%~1" == "-i"             set "ARG_BUILD_INSTALLERS=!ARG_BUILD_INSTALLERS! %~2" & shift
if /i "%~1" == "--installer"    set "ARG_BUILD_INSTALLERS=!ARG_BUILD_INSTALLERS! %~2" & shift
if /i "%~1" == "-arch"          set "ARCH=%~2" & shift
if /i "%~1" == "--architecture" set "ARCH=%~2" & shift
shift
goto :parse_args
:done_args

if defined ARG_BUILD_INSTALLERS set "ARG_BUILD_INSTALLERS=%ARG_BUILD_INSTALLERS:all=ifw wix%"

:: understand toolchain arch (host_target) and Qt prefix path
if not defined ARCH set "ARCH=%PROCESSOR_ARCHITECTURE%"
if /i "%ARCH%" == "x64" set "ARCH=amd64"
if /i "%ARCH%" == "amd64" (
    if /i "%PROCESSOR_ARCHITECTURE%" == "AMD64" set "_vcvars_arg=amd64"
    if /i "%PROCESSOR_ARCHITECTURE%" == "x86"   set "_vcvars_arg=x86_amd64"
    set "_qt_postfix_arg=64"
)
if /i "%ARCH%" == "arm64" (
    if /i "%PROCESSOR_ARCHITECTURE%" == "AMD64" set "_vcvars_arg=amd64_arm64"
    if /i "%PROCESSOR_ARCHITECTURE%" == "x86"   set "_vcvars_arg=x86_arm64"
    set "_qt_postfix_arg=arm64"
)
if not defined _vcvars_arg  (
    echo ERROR: Unsupported architecture "%ARCH%"
    goto :fail
)

if not defined QT_VERSION  set "QT_VERSION=6.*"
if not defined QIF_VERSION set "QIF_VERSION=*"

set "_qt_bases=%USERPROFILE%\Qt C:\Qt"
if defined QT_INSTALL_DIR set "_qt_bases=%QT_INSTALL_DIR%\Qt %_qt_bases%"

:: search over Qt dirs to find framework and tools paths
for %%B in (%_qt_bases%) do (
    if exist "%%~B" (
        for /d %%D in (%%~B\%QT_VERSION%) do (
            if not defined _qt_root_path (
                set "_qt_root_path=%%D"
            ) else (
                for %%I in (!_qt_root_path!) do call :compare_versions "%%~nxI" "%%~nxD"
                if errorlevel 1 set "_qt_root_path=%%D"
            )
        )

        for /d %%D in (%%~B\Tools\QtInstallerFramework\%QIF_VERSION%) do (
            if not defined _qif_root_path (
                set "_qif_root_path=%%D"
            ) else (
                for %%I in (!_qif_root_path!) do call :compare_versions "%%~nxI" "%%~nxD"
                if errorlevel 1 set "_qif_root_path=%%D"
            )
        )

        if exist "%%~B\Tools\Ninja" set "PATH=%PATH%;%%~B\Tools\Ninja"
    )
)

if not defined QT_ROOT_PATH  set "QT_ROOT_PATH=%_qt_root_path%"
if not defined QIF_ROOT_PATH set "QIF_ROOT_PATH=%_qif_root_path%"

:: use vswhere to find path to vcvarsall.bat
if not defined VCINSTALLDIR (
    if not defined VS_INSTALLER_PATH set "VS_INSTALLER_PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "%VS_INSTALLER_PATH%" (
        if defined "%VS_INSTALLATION_VERSION%" (
            set "_version=-version [%VS_INSTALLATION_VERSION%]"
        ) else (
            set "_version=-latest"
        )
        for /f "usebackq tokens=*" %%I in (
            `"%VS_INSTALLER_PATH%" -products * %_version% -property resolvedInstallationPath`
        ) do (
            if not defined VCVARS_PATH set "VCVARS_PATH=%%I\VC\Auxiliary\Build\vcvarsall.bat"
        )
    )
)

:: setup MSVC toolchain using vsvarsall.bat
if exist "%VCVARS_PATH%" (
    @echo on
    call "%VCVARS_PATH%" %_vcvars_arg%
    @echo off
    if errorlevel 1 goto :fail
)

:: build project and installers
@echo on
cmake -S "%PROJECT_DIR%" -B "%BUILD_DIR%" -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=%QT_ROOT_PATH%\msvc2022_%_qt_postfix_arg%" "-DCMAKE_VS_GLOBALS=UseMultiToolTask=true;EnforceProcessCountAcrossBuilds=true" || goto :fail
cmake --build "%BUILD_DIR%" --config Release -- /m  || goto :fail
@echo off
for %%I in (%ARG_BUILD_INSTALLERS%) do (
    if /i "%%I" == "ifw" call :do_ifw
    if /i "%%I" == "wix" call :do_wix
    if errorlevel 1 goto :fail
)
goto :eof

:: bakes IFW installer
:do_ifw
@echo on
cd "%BUILD_DIR%" && cpack -G IFW -D "QTIFWDIR=%QIF_ROOT_PATH%" || goto :fail
@echo off
goto :eof

:: bakes WIX installer
:do_wix
@echo on
cd "%BUILD_DIR%" && cpack -G WIX -D "WIX_BIN_DIR=%WIX_ROOT_PATH%" || goto :fail
@echo off
goto :eof

:fail
exit /b 1

:: compares any two versions
:compare_versions
set "_left=%~1"
set "_right=%~2"

set "_left_temp=%_left%"
set "_right_temp=%_right%"

:seg_loop
set "_ls=0"
for /f "tokens=1* delims=." %%A in ("%_left_temp%") do (
    set "_ls=%%A" & set "_left_temp=%%B"
)
set "_rs=0"
for /f "tokens=1* delims=." %%A in ("%_right_temp%") do (
    set "_rs=%%A" & set "_right_temp=%%B"
)

if %_rs% GTR %_ls% exit /b 1
if %_rs% LSS %_ls% exit /b -1
if defined _left_temp goto :seg_loop
if defined _right_temp goto :seg_loop
exit /b 0
