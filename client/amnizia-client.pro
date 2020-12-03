QT += widgets core gui network xml

TARGET = amnezia-client
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS


win32 {

#win32-g++ {
#   QMAKE_CXXFLAGS += -Werror
#}
#win32-msvc*{
#   QMAKE_CXXFLAGS += /WX
#}

FORMS    += ui/mainwindow.ui

RESOURCES += \
    res.qrc

OTHER_FILES += platform_win/vpnclient.rc
RC_FILE = platform_win/vpnclient.rc

HEADERS += publib/winhelp.h

SOURCES += publib/winhelp.cpp

CONFIG -= embed_manifest_exe
DEFINES += _CRT_SECURE_NO_WARNINGS VPNCLIENT_TAPSIGNED
#QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

VERSION = 1.1.1.1
QMAKE_TARGET_COMPANY = "AmneziaVPN"
QMAKE_TARGET_PRODUCT = "AmneziaVPN"

CONFIG -= embed_manifest_exe

LIBS += -luser32 \
        -lrasapi32 \
        -lshlwapi \
        -liphlpapi \
        -lws2_32 \
        -liphlpapi \
        -lgdi32


MT_PATH = \"C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/bin/x64/mt.exe\"
WIN_PWD = $$replace(PWD, /, \\)
OUT_PWD_WIN = $$replace(OUT_PWD, /, \\)

!win32-g++: QMAKE_POST_LINK = "$$MT_PATH -manifest $$quote($$WIN_PWD\\platform_win\\$$basename(TARGET).exe.manifest) -outputresource:$$quote($$OUT_PWD_WIN\\$(DESTDIR_TARGET);1)"
   else: QMAKE_POST_LINK = "$$MT_PATH -manifest $$PWD/platform_win/$$basename(TARGET).exe.manifest -outputresource:$$OUT_PWD/$(DESTDIR_TARGET)"
}

macx {

OBJECTIVE_HEADERS +=
OBJECTIVE_SOURCES += publib/macos_functions.mm

HEADERS += \

SOURCES += \

QMAKE_OBJECTIVE_CFLAGS += -F/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.12.sdk/System/Library/Frameworks

FORMS    += ui/mainwindow_mac.ui

LIBS += -framework CoreServices -framework Foundation -framework AppKit

RESOURCES += \
    res_mac.qrc

ICON   = images/main.icns
}

SOURCES  += main.cpp\
            core/router.cpp \
            publib/debug.cpp \
            publib/runguard.cpp \
            publib/winhelp.cpp \
            ui/Controls/SlidingStackedWidget.cpp \
            ui/mainwindow.cpp \
            ui/customshadoweffect.cpp

HEADERS  += ui/mainwindow.h \
   core/router.h \
   publib/debug.h \
   publib/runguard.h \
   publib/winhelp.h \
   ui/customshadoweffect.h \
   ui/Controls/SlidingStackedWidget.h

FORMS    += ui/mainwindow.ui


TRANSLATIONS = translations/amneziavpn.en.ts \
    translations/amneziavpn.ru.ts



win32: LIBS += -L$$PWD/../../../../../../../OpenSSL-Win32/lib/ -llibcrypto

