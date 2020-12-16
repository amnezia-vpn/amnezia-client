TARGET   = AmneziaVPN-service
TEMPLATE = app
CONFIG   += console qt
QT = core network 

HEADERS = \
        server.h
SOURCES = \
        server.cpp \
        main.cpp

include(../src/qtservice.pri)

CONFIG(release, debug|release) {
    DESTDIR = $$PWD/../../../AmneziaVPN-build/server/release
    MOC_DIR = $$DESTDIR
    OBJECTS_DIR = $$DESTDIR
    RCC_DIR = $$DESTDIR
}
