QT += widgets core gui network xml

TARGET = AmneziaVPN
TEMPLATE = app
#CONFIG += console

DEFINES += QT_DEPRECATED_WARNINGS

HEADERS  += \
            core/router.h \
            debug.h \
            defines.h \
            runguard.h \
            ui/Controls/SlidingStackedWidget.h \
            ui/mainwindow.h \

SOURCES  += \
            core/router.cpp \
            debug.cpp \
            main.cpp \
            runguard.cpp \
            ui/Controls/SlidingStackedWidget.cpp \
            ui/mainwindow.cpp \


FORMS    += ui/mainwindow.ui

RESOURCES += \
            resources.qrc

TRANSLATIONS = \
            translations/amneziavpn.en.ts \
            translations/amneziavpn.ru.ts

win32 {

OTHER_FILES += platform_win/vpnclient.rc
RC_FILE = platform_win/vpnclient.rc

HEADERS +=
SOURCES +=

#CONFIG -= embed_manifest_exe
#DEFINES += _CRT_SECURE_NO_WARNINGS VPNCLIENT_TAPSIGNED
#QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"

VERSION = 1.1.1.1
QMAKE_TARGET_COMPANY = "AmneziaVPN"
QMAKE_TARGET_PRODUCT = "AmneziaVPN"

#CONFIG -= embed_manifest_exe

LIBS += \
        -luser32 \
        -lrasapi32 \
        -lshlwapi \
        -liphlpapi \
        -lws2_32 \
        -liphlpapi \
        -lgdi32

#LIBS += -L$$PWD/../../../../../../../OpenSSL-Win32/lib/ -llibcrypto

#MT_PATH = \"C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/bin/x64/mt.exe\"
#WIN_PWD = $$replace(PWD, /, \\)
#OUT_PWD_WIN = $$replace(OUT_PWD, /, \\)

#!win32-g++: QMAKE_POST_LINK = "$$MT_PATH -manifest $$quote($$WIN_PWD\\platform_win\\$$basename(TARGET).exe.manifest) -outputresource:$$quote($$OUT_PWD_WIN\\$(DESTDIR_TARGET);1)"
#   else: QMAKE_POST_LINK = "$$MT_PATH -manifest $$PWD/platform_win/$$basename(TARGET).exe.manifest -outputresource:$$OUT_PWD/$(DESTDIR_TARGET)"

}


macx {
ICON   = $$PWD/images/app.icns
}
