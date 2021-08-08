INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD

CONFIG += c++17

INCLUDEPATH += $$PWD/include/external

win32 {
   QMAKE_CXXFLAGS += -bigobj
   LIBS += \
       -lcrypt32 \

   !contains(QMAKE_TARGET.arch, x86_64) {
      message("Windows x86 build")
      INCLUDEPATH += $$PWD/windows/x86_64
      HEADERS += $$PWD/windows/x86/botan_all.h
      SOURCES += $$PWD/windows/x86/botan_all.cpp
   }
   else {
      message("Windows x86_64 build")
      INCLUDEPATH += $$PWD/windows/x86_64
      HEADERS += $$PWD/windows/x86_64/botan_all.h
      SOURCES += $$PWD/windows/x86_64/botan_all.cpp
   }
}

macx {
    message("macOS build")
    HEADERS += $$PWD/macos/botan_all.h
    SOURCES += $$PWD/macos/botan_all.cpp
}

linux-g++ {
    message("Linux build")
    HEADERS += $$PWD/linux/botan_all.h
    SOURCES += $$PWD/linux/botan_all.cpp
}

android {
   for (abi, ANDROID_ABIS): {
      equals(ANDROID_TARGET_ARCH,$$abi) {
         message("Android build for ANDROID_TARGET_ARCH" $$abi)
         HEADERS += $$PWD/android/$${abi}/botan_all.h
         SOURCES += $$PWD/android/$${abi}/botan_all.cpp
      }
   }
}

