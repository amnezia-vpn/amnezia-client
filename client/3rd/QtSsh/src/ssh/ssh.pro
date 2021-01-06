TARGET = QtSsh

load(qt_module)

DEFINES += QTCSSH_LIBRARY

include($$PWD/ssh.pri)
include($$PWD/../botan/botan.pri)
