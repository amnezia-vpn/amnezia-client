INCLUDEPATH += $$PWD/include
DEPENDPATH += $$PWD

CONFIG += c++17

INCLUDEPATH += $$PWD/include/external

win32 {
   QMAKE_CXXFLAGS += -bigobj
   LIBS += \
       -lcrypt32 \

   !contains(QMAKE_TARGET.arch, x86_64) {
      INCLUDEPATH += $$PWD/windows/x86
      HEADERS += $$PWD/windows/x86/botan_all.h
      SOURCES += $$PWD/windows/x86/botan_all.cpp
   }
   else {
      INCLUDEPATH += $$PWD/windows/x86_64
      HEADERS += $$PWD/windows/x86_64/botan_all.h
      SOURCES += $$PWD/windows/x86_64/botan_all.cpp
   }
}

macx:!ios {
    INCLUDEPATH += $$PWD/macos
    HEADERS += $$PWD/macos/botan_all.h
    SOURCES += $$PWD/macos/botan_all.cpp
}

linux-g++ {
    INCLUDEPATH += $$PWD/linux
    HEADERS += $$PWD/linux/botan_all.h
    SOURCES += $$PWD/linux/botan_all.cpp

    LIBS += -ldl
}

android {
    versionAtLeast(QT_VERSION, 6.0.0) {
        # We need to include qtprivate api's
        # As QAndroidBinder is not yet implemented with a public api
        QT+=core-private
        ANDROID_ABIS=ANDROID_TARGET_ARCH

        INCLUDEPATH += $$PWD/android/$${ANDROID_TARGET_ARCH}
        HEADERS += $$PWD/android/$${ANDROID_TARGET_ARCH}/botan_all.h
        SOURCES += $$PWD/android/$${ANDROID_TARGET_ARCH}/botan_all.cpp
    }
    else {
        QT += androidextras

        for (abi, ANDROID_ABIS): {
           equals(ANDROID_TARGET_ARCH,$$abi) {
              INCLUDEPATH += $$PWD/android/$${abi}
              HEADERS += $$PWD/android/$${abi}/botan_all.h
              SOURCES += $$PWD/android/$${abi}/botan_all.cpp
           }
        }
    }
}

ios: {
    CONFIG(iphoneos, iphoneos|iphonesimulator) {
        contains(QT_ARCH, arm64) {
            INCLUDEPATH += $$PWD/ios/iphone
            HEADERS += $$PWD/ios/iphone/botan_all.h
            SOURCES += $$PWD/ios/iphone/botan_all.cpp
        } else {
            message("Building for iOS/ARM v7 (32-bit) architecture")
            ARCH_TAG = "ios_armv7"
        }
    }
  
  CONFIG(iphonesimulator, iphoneos|iphonesimulator) {
      INCLUDEPATH += $$PWD/ios/iphone
      HEADERS += $$PWD/ios/iphone/botan_all.h
      SOURCES += $$PWD/ios/iphone/botan_all.cpp
  }

#    CONFIG(iphonesimulator, iphoneos|iphonesimulator) {
#        INCLUDEPATH += $$PWD/ios/simulator
#        HEADERS += $$PWD/ios/simulator/botan_all.h
#        SOURCES += $$PWD/ios/simulator/botan_all.cpp
#    }
}
