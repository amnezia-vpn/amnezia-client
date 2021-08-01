INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD

win32 {
   INCLUDEPATH += $$PWD/include/windows/botan-2

   !contains(QMAKE_TARGET.arch, x86_64) {
           message("x86 build")
           INCLUDEPATH += $$PWD/include/windows/x86
           CONFIG(release, debug|release): LIBS += -L$$PWD/lib/windows/x86 -lbotan
           CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/windows/x86 -lbotand
       }
   else {
           message("x86_64 build")
           INCLUDEPATH += $$PWD/include/windows/x86_64
           CONFIG(release, debug|release): LIBS += -L$$PWD/lib/windows/x86_64 -lbotan
           CONFIG(debug, debug|release): LIBS += -L$$PWD/lib/windows/x86_64 -lbotand
   }
}


android {
   INCLUDEPATH += $$PWD/include/android/botan-2
   for (abi, ANDROID_ABIS): {
      equals(ANDROID_TARGET_ARCH,$$abi) {
         message( "ANDROID_TARGET_ARCH" $$abi)
         INCLUDEPATH += $$PWD/include/android/$${abi}
         LIBS += -L$$PWD/lib/android/$${abi} -lbotan-2
         ANDROID_EXTRA_LIBS += $$PWD/lib/android/$${abi}/libbotan-2.so
      }
   }
}
