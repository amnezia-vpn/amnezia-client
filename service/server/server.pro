TARGET   = AmneziaVPN-service
TEMPLATE = app
CONFIG   += console qt no_batch
QT += core network

HEADERS = \
        ../../client/message.h \
        ../../client/utils.h \
        localserver.h \
        log.h \
        systemservice.h

SOURCES = \
        ../../client/message.cpp \
        ../../client/utils.cpp \
        localserver.cpp \
        log.cpp \
        main.cpp \
        systemservice.cpp

include(../src/qtservice.pri)

#CONFIG(release, debug|release) {
#    DESTDIR = $$PWD/../../../AmneziaVPN-build/server/release
#    MOC_DIR = $$DESTDIR
#    OBJECTS_DIR = $$DESTDIR
#    RCC_DIR = $$DESTDIR
#}

INCLUDEPATH += "$$PWD/../../client"
