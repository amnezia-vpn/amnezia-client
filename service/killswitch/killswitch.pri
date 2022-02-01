INCLUDEPATH+= $$PWD
DEPENDPATH += $$PWD
QT         += core network
HEADERS += \
                $$PWD/leakdetector.h \
                $$PWD/windowsfirewall.h \
                $$PWD/ipaddress.h \
                $$PWD/windowscommons.h

    SOURCES += \
                $$PWD/leakdetector.cpp \
                $$PWD/windowsfirewall.cpp \
                $$PWD/ipaddress.cpp \
                $$PWD/windowscommons.cpp

   # TARGET = libkillswitch

