TARGET   = wireguard-service
TEMPLATE = app
CONFIG   += console
CONFIG -= app_bundle
CONFIG -= qt
LIBS += \
        -luser32 \
        -lrasapi32 \
        -lshlwapi \
        -liphlpapi \
        -lws2_32 \
        -liphlpapi \
        -lgdi32 \
        -lAdvapi32 \
        -lKernel32

HEADERS = \
    wireguardtunnelservice.h

SOURCES = \
           main.cpp \
           wireguardtunnelservice.cpp

