#exists(config.pri):infile(config.pri, SOLUTIONS_LIBRARY, yes): CONFIG += qtservice-uselib
TEMPLATE += fakelib
QTSERVICE_LIBNAME = QtSolutions_Service-head
CONFIG(debug, debug|release) {
	mac:QTSERVICE_LIBNAME = $$member(QTSERVICE_LIBNAME, 0)_debug
   	else:win32:QTSERVICE_LIBNAME = $$member(QTSERVICE_LIBNAME, 0)d
}
TEMPLATE -= fakelib
QTSERVICE_LIBDIR = $$PWD/lib
unix:qtservice-uselib:!qtservice-buildlib:QMAKE_RPATHDIR += $$QTSERVICE_LIBDIR
