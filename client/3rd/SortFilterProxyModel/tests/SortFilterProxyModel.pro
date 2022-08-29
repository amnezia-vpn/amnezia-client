TEMPLATE = app
TARGET = tst_sortfilterproxymodel
QT += qml quick
CONFIG += c++11 warn_on qmltestcase qml_debug no_keywords

include(../SortFilterProxyModel.pri)

HEADERS += \
    indexsorter.h \
    testroles.h

SOURCES += \
    tst_sortfilterproxymodel.cpp \
    indexsorter.cpp \
    testroles.cpp

OTHER_FILES += \
    tst_rangefilter.qml \
    tst_indexfilter.qml \
    tst_sourceroles.qml \
    tst_sorters.qml \
    tst_helpers.qml \
    tst_builtins.qml \
    tst_rolesorter.qml \
    tst_stringsorter.qml \
    tst_proxyroles.qml \
    tst_joinrole.qml \
    tst_switchrole.qml \
    tst_expressionrole.qml \
    tst_filtercontainerattached.qml \
    tst_filtercontainers.qml \
    tst_regexprole.qml \
    tst_filtersorter.qml \
    tst_filterrole.qml \
    tst_delayed.qml \
    tst_sortercontainerattached.qml
