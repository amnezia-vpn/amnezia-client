#!/bin/bash
# Build using build_windows.bat
echo "Build script started ..."

echo -e 'call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\BuildTools\\Common7\\Tools\\VsDevCmd.bat"\n' > winbuild.bat
echo -e 'set BUILD_ARCH="64"\n' >> winbuild.bat
echo -e 'set QT_BIN_DIR="c:\\Qt\\$QT_VERSION\\msvc2017_64\\bin"\n' >> winbuild.bat
echo -e 'set QIF_BIN_DIR="c:\\Qt\\Tools\\QtInstallerFramework\\4.1\\bin"\n' >> winbuild.bat
echo -e 'call "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Auxiliary\\Build\\vcvars%BUILD_ARCH:"=%.bat"\n' >> winbuild.bat
cat winbuild.bat
echo -e 'set WIN_CERT_PW=$WIN_CERT_PW\n' >> winbuild.bat
echo -e 'call deploy\\\build_windows.bat' >> winbuild.bat
cmd //c winbuild.bat
