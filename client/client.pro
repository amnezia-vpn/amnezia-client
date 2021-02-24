QT += widgets core gui network xml remoteobjects

TARGET = AmneziaVPN
TEMPLATE = app
#CONFIG += console

DEFINES += QT_DEPRECATED_WARNINGS

include("3rd/QtSsh/src/ssh/ssh.pri")
include("3rd/QtSsh/src/botan/botan.pri")

HEADERS  += \
    ../ipc/ipc.h \
    core/defs.h \
    core/errorstrings.h \
    core/ipcclient.h \
    core/openvpnconfigurator.h \
    core/servercontroller.h \
    debug.h \
    defines.h \
    managementserver.h \
    protocols/shadowsocksvpnprotocol.h \
    runguard.h \
    settings.h \
    ui/Controls/SlidingStackedWidget.h \
    ui/mainwindow.h \
   ui/qautostart.h \
    utils.h \
    vpnconnection.h \
    protocols/vpnprotocol.h \
    protocols/openvpnprotocol.h \

SOURCES  += \
    core/ipcclient.cpp \
    core/openvpnconfigurator.cpp \
    core/servercontroller.cpp \
    debug.cpp \
    main.cpp \
    managementserver.cpp \
    protocols/shadowsocksvpnprotocol.cpp \
    runguard.cpp \
    settings.cpp \
    ui/Controls/SlidingStackedWidget.cpp \
    ui/mainwindow.cpp \
   ui/qautostart.cpp \
    utils.cpp \
    vpnconnection.cpp \
    protocols/vpnprotocol.cpp \
    protocols/openvpnprotocol.cpp \

FORMS    += ui/mainwindow.ui

RESOURCES += \
    resources.qrc

TRANSLATIONS = \
    translations/amneziavpn_ru.ts

#CONFIG(release, debug|release) {
#    DESTDIR = $$PWD/../../AmneziaVPN-build/client/release
#    MOC_DIR = $$DESTDIR
#    OBJECTS_DIR = $$DESTDIR
#    RCC_DIR = $$DESTDIR
#}

win32 {
    OTHER_FILES += platform_win/vpnclient.rc
    RC_FILE = platform_win/vpnclient.rc

    HEADERS += \
       ui/framelesswindow.h \

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

    HEADERS  += ui/macos_util.h
    SOURCES  += ui/macos_util.mm

    LIBS += -framework Cocoa -framework ApplicationServices -framework CoreServices -framework Foundation -framework AppKit
}

REPC_REPLICA += ../ipc/ipcinterface.rep

