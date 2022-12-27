include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/include/external
)

if(WIN32)
    add_compile_definitions(MVPN_WINDOWS)
    add_compile_options(/bigobj)
    set(LIBS ${LIBS}
        crypt32
    )

    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        include_directories(${CMAKE_CURRENT_LIST_DIR}/windows/x86_64)
        set(HEADERS ${HEADERS} ${CMAKE_CURRENT_LIST_DIR}/windows/x86_64/botan_all.h)
        set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/windows/x86_64/botan_all.cpp)
    else()
        include_directories(${CMAKE_CURRENT_LIST_DIR}/windows/x86)
        set(HEADERS ${HEADERS} ${CMAKE_CURRENT_LIST_DIR}/windows/x86/botan_all.h)
        set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/windows/x86/botan_all.cpp)
    endif()
endif()

if(APPLE AND NOT IOS)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/macos)
    set(HEADERS ${HEADERS} ${CMAKE_CURRENT_LIST_DIR}/macos/botan_all.h)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/macos/botan_all.cpp)
endif()

if(LINUX)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/linux)
    set(HEADERS ${HEADERS} ${CMAKE_CURRENT_LIST_DIR}/linux/botan_all.h)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/linux/botan_all.cpp)
    set(LIBS ${LIBS} dl)
endif()

if(ANDROID)
    # We need to include qtprivate api's
    # As QAndroidBinder is not yet implemented with a public api
    set(LIBS ${LIBS} Qt6::CorePrivate)

    message("botan target arch ${CMAKE_ANDROID_ARCH_ABI}")
    set(abi ${CMAKE_ANDROID_ARCH_ABI})

    include_directories(${CMAKE_CURRENT_LIST_DIR}/android/${abi})
    link_directories(${CMAKE_CURRENT_LIST_DIR}/android/${abi})
    set(HEADERS ${HEADERS} ${CMAKE_CURRENT_LIST_DIR}/android/${abi}/botan_all.h)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/android/${abi}/botan_all.cpp)

endif()

if(IOS)
    # CONFIG(iphoneos, iphoneos|iphonesimulator) {
    #     contains(QT_ARCH, arm64) {
    #         INCLUDEPATH += $$PWD/ios/iphone
    #         HEADERS += $$PWD/ios/iphone/botan_all.h
    #         SOURCES += $$PWD/ios/iphone/botan_all.cpp
    #     } else {
    #         message("Building for iOS/ARM v7 (32-bit) architecture")
    #         ARCH_TAG = "ios_armv7"
    #     }
    # }

    # CONFIG(iphonesimulator, iphoneos|iphonesimulator) {
    # INCLUDEPATH += $$PWD/ios/iphone
    # HEADERS += $$PWD/ios/iphone/botan_all.h
    # SOURCES += $$PWD/ios/iphone/botan_all.cpp
    # }

    include_directories(${CMAKE_CURRENT_LIST_DIR}/ios/iphone)
    set(HEADERS ${HEADERS} ${CMAKE_CURRENT_LIST_DIR}/ios/iphone/botan_all.h)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/ios/iphone/botan_all.cpp)




endif()
