INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD

win32 {
   CONFIG(release, debug|release): LIBS += -L$$PWD/lib/windows_x64 -lbotan
   win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/windows_x64 -lbotand
}
