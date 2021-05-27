QT += widgets core gui network xml remoteobjects

TARGET = AmneziaVPN
TEMPLATE = app
#CONFIG += console

DEFINES += QT_DEPRECATED_WARNINGS

include("3rd/QtSsh/src/ssh/ssh.pri")
include("3rd/QtSsh/src/botan/botan.pri")
include("3rd/SingleApplication/singleapplication.pri")
include("3rd/QRCodeGenerator/QRCodeGenerator.pri")

HEADERS  += \
    ../ipc/ipc.h \
   configurators/cloak_configurator.h \
   configurators/shadowsocks_configurator.h \
   configurators/vpn_configurator.h \
    core/defs.h \
    core/errorstrings.h \
    core/ipcclient.h \
    configurators/openvpn_configurator.h \
   core/scripts_registry.h \
   core/server_defs.h \
    core/servercontroller.h \
    debug.h \
    defines.h \
    managementserver.h \
   protocols/openvpnovercloakprotocol.h \
   protocols/protocols_defs.h \
    protocols/shadowsocksvpnprotocol.h \
    settings.h \
    ui/Controls/SlidingStackedWidget.h \
    ui/mainwindow.h \
   ui/qautostart.h \
   ui/server_widget.h \
   ui/sites_model.h \
    utils.h \
    vpnconnection.h \
    protocols/vpnprotocol.h \
    protocols/openvpnprotocol.h \

SOURCES  += \
   configurators/cloak_configurator.cpp \
   configurators/shadowsocks_configurator.cpp \
   configurators/vpn_configurator.cpp \
   core/errorstrings.cpp \
    core/ipcclient.cpp \
    configurators/openvpn_configurator.cpp \
   core/scripts_registry.cpp \
   core/server_defs.cpp \
    core/servercontroller.cpp \
    debug.cpp \
    main.cpp \
    managementserver.cpp \
   protocols/openvpnovercloakprotocol.cpp \
   protocols/protocols_defs.cpp \
    protocols/shadowsocksvpnprotocol.cpp \
    settings.cpp \
    ui/Controls/SlidingStackedWidget.cpp \
    ui/mainwindow.cpp \
   ui/qautostart.cpp \
   ui/server_widget.cpp \
   ui/sites_model.cpp \
    utils.cpp \
    vpnconnection.cpp \
    protocols/vpnprotocol.cpp \
    protocols/openvpnprotocol.cpp \

FORMS    += ui/mainwindow.ui \
   ui/server_widget.ui

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

