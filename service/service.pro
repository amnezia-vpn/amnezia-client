!ios:!android {
    TEMPLATE=subdirs
    CONFIG += ordered
    include(common.pri)
    qtservice-uselib:SUBDIRS=buildlib
    SUBDIRS+=server
}
win32 {
    SUBDIRS+=wireguard-service
}
