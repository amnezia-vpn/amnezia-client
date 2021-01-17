TARGET   = post-uninstall
TEMPLATE = app
CONFIG   += console qt
QT = core

SOURCES = \
        main.cpp

#CONFIG(release, debug|release) {
#    DESTDIR = $$PWD/../../../AmneziaVPN-build/post-uninstall/release
#    MOC_DIR = $$DESTDIR
#    OBJECTS_DIR = $$DESTDIR
#    RCC_DIR = $$DESTDIR
#}

INCLUDEPATH += "$$PWD/../../client"
