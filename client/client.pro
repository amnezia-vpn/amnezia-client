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

CONFIG(release, debug|release) {
    DESTDIR = $$PWD/../../AmneziaVPN-build/client/release
    MOC_DIR = $$DESTDIR
    OBJECTS_DIR = $$DESTDIR
    RCC_DIR = $$DESTDIR
}

win32 {
    OTHER_FILES += platform_win/vpnclient.rc
    RC_FILE = platform_win/vpnclient.rc

    HEADERS +=
    SOURCES +=

    VERSION = 1.1.1.1
    QMAKE_TARGET_COMPANY = "AmneziaVPN"
    QMAKE_TARGET_PRODUCT = "AmneziaVPN"

    LIBS += \
        -luser32 \
        -lrasapi32 \
        -lshlwapi \
        -liphlpapi \
        -lws2_32 \
        -liphlpapi \
        -lgdi32

    #LIBS += -L$$PWD/../../../../../../../OpenSSL-Win32/lib/ -llibcrypto
}

macx {
    ICON   = $$PWD/images/app.icns
}
