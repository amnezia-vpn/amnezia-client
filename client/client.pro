QT += widgets core gui network xml

TARGET = AmneziaVPN
TEMPLATE = app
CONFIG += console

DEFINES += QT_DEPRECATED_WARNINGS

include("3rd/QtSsh/src/ssh/ssh.pri")
include("3rd/QtSsh/src/botan/botan.pri")

HEADERS  += \
            communicator.h \
            core/openvpnconfigurator.h \
            core/router.h \
            core/servercontroller.h \
            debug.h \
            defines.h \
            localclient.h \
            managementserver.h \
            message.h \
            openvpnprotocol.h \
            runguard.h \
            settings.h \
            ui/Controls/SlidingStackedWidget.h \
            ui/framelesswindow.h \
            ui/mainwindow.h \
            utils.h \
            vpnconnection.h \
            vpnprotocol.h \

SOURCES  += \
            communicator.cpp \
            core/openvpnconfigurator.cpp \
            core/router.cpp \
            core/servercontroller.cpp \
            debug.cpp \
            localclient.cpp \
            main.cpp \
            managementserver.cpp \
            message.cpp \
            openvpnprotocol.cpp \
            runguard.cpp \
            settings.cpp \
            ui/Controls/SlidingStackedWidget.cpp \
            ui/mainwindow.cpp \
            utils.cpp \
            vpnconnection.cpp \
            vpnprotocol.cpp \

FORMS    += ui/mainwindow.ui

RESOURCES += \
            resources.qrc

TRANSLATIONS = \
            translations/amneziavpn_ru.ts

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
    SOURCES += \
       ui/framelesswindow.cpp

    VERSION = 1.0.0.0
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

   HEADERS +=
   SOURCES += \
      ui/framelesswindow.mm

}
