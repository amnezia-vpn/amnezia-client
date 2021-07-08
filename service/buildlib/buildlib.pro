TEMPLATE=lib
CONFIG += qt dll qtservice-buildlib
mac:CONFIG += absolute_library_soname
win32|mac:!wince*:!win32-msvc:!macx-xcode:CONFIG += debug_and_release build_all
include(../src/qtservice.pri)
TARGET = $$QTSERVICE_LIBNAME
DESTDIR = $$QTSERVICE_LIBDIR
win32 {
    DLLDESTDIR = $$[QT_INSTALL_BINS]
    QMAKE_DISTCLEAN += $$[QT_INSTALL_BINS]\\$${QTSERVICE_LIBNAME}.dll
}
target.path = $$DESTDIR
INSTALLS += target
