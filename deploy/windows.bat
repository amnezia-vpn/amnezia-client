 @ECHO OFF

CHCP 1252

SET QT_BIN_DIR="c:\Devel\Qt\5.14.2\msvc2017\bin"
SET QIF_BIN_DIR="c:\Devel\Qt\Tools\QtInstallerFramework\4.0\bin"

set APP_NAME=AmneziaVPN
set APP_FILENAME=%APP_NAME:"=%.exe
set APP_DOMAIN=org.amneziavpn.package
set LAUNCH_DIR=%cd%
set TOP_DIR=%LAUNCH_DIR:"=%\..
set RELEASE_DIR=%TOP_DIR:"=%\..\%APP_NAME:"=%-build
set OUT_APP_DIR=%RELEASE_DIR:"=%\client\release
set DEPLOY_DATA_DIR=%LAUNCH_DIR:"=%\data\windows
set INSTALLER_DATA_DIR=%RELEASE_DIR:"=%\installer\packages\%APP_DOMAIN:"=%\data
set PRO_FILE_PATH=%TOP_DIR:"=%\%APP_NAME:"=%.pro
set QMAKE_STASH_FILE=%TOP_DIR:"=%\.qmake_stash
set TARGET_FILENAME=%TOP_DIR:"=%\%APP_NAME:"=%.exe

echo "Environment:"
echo "APP_FILENAME:			%APP_FILENAME%"
echo "LAUNCH_DIR:			%LAUNCH_DIR%"
echo "TOP_DIR:			%TOP_DIR%"
echo "RELEASE_DIR:			%RELEASE_DIR%"
echo "OUT_APP_DIR:			%OUT_APP_DIR%"
echo "DEPLOY_DATA_DIR: 		%DEPLOY_DATA_DIR%"
echo "INSTALLER_DATA_DIR: 		%INSTALLER_DATA_DIR%"
echo "PRO_FILE_PATH: 		%PRO_FILE_PATH%"
echo "QMAKE_STASH_FILE: 		%QMAKE_STASH_FILE%"
echo "TARGET_FILENAME: 		%TARGET_FILENAME%"

echo "Cleanup..."
Rmdir /Q /S %RELEASE_DIR%
Del %QMAKE_STASH_FILE%
Del %TARGET_FILENAME%


cd %TOP_DIR%
"%QT_BIN_DIR:"=%\qmake" %PRO_FILE_PATH% -spec win32-msvc
set CL=/MP
nmake /A /NOLOGO
del "%OUT_APP_DIR:"=%\*.obj"
del "%OUT_APP_DIR:"=%\*.cpp"
del "%OUT_APP_DIR:"=%\*.h"
del "%OUT_APP_DIR:"=%\*.res"
del "%OUT_APP_DIR:"=%\*.o"
del "%OUT_APP_DIR:"=%\*.moc"
del "%OUT_APP_DIR:"=%\*.lib"
del "%OUT_APP_DIR:"=%\*.exp"
echo "Deploying..."
"%QT_BIN_DIR:"=%\windeployqt" --release --force --no-translations "%OUT_APP_DIR:"=%\%APP_FILENAME:"=%"
echo "Copying deploy data..."
xcopy %DEPLOY_DATA_DIR% 											%OUT_APP_DIR%  /s /e /y /i /f
copy "%RELEASE_DIR:"=%\server\release\%APP_NAME:"=%-service.exe"	%OUT_APP_DIR%
copy "%RELEASE_DIR:"=%\post-uninstall\release\post-uninstall.exe"	%OUT_APP_DIR%

cd %LAUNCH_DIR%
xcopy %LAUNCH_DIR:"=%\installer 									%RELEASE_DIR:"=%\installer /s /e /y /i /f
mkdir %INSTALLER_DATA_DIR%

cd %OUT_APP_DIR%
echo "Compressing data..."
"%QIF_BIN_DIR:"=%\archivegen" -c 9 %INSTALLER_DATA_DIR:"=%\%APP_NAME:"=%.7z ./

cd "%RELEASE_DIR:"=%\installer"
echo "Creating installer..."
"%QIF_BIN_DIR:"=%\binarycreator" --offline-only -v -c config\windows.xml -p packages -f %TARGET_FILENAME%


cd %LAUNCH_DIR%
echo "Finished, see %TARGET_FILENAME%"
