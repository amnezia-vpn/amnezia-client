message("Client android ${CMAKE_ANDROID_ARCH_ABI} build")

set(APP_ANDROID_MIN_SDK 26)
set(ANDROID_PLATFORM "android-${APP_ANDROID_MIN_SDK}" CACHE STRING
    "The minimum API level supported by the application or library" FORCE)

# set QTP0002 policy: target properties that specify Android-specific paths may contain generator expressions
qt_policy(SET QTP0002 NEW)

set_target_properties(${PROJECT} PROPERTIES
    QT_ANDROID_VERSION_NAME ${CMAKE_PROJECT_VERSION}
    QT_ANDROID_VERSION_CODE ${APP_ANDROID_VERSION_CODE}
    QT_ANDROID_MIN_SDK_VERSION ${APP_ANDROID_MIN_SDK}
    QT_ANDROID_TARGET_SDK_VERSION 34
    QT_ANDROID_SDK_BUILD_TOOLS_REVISION 34.0.0
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)

set(QT_ANDROID_MULTI_ABI_FORWARD_VARS "QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL;CMAKE_BUILD_TYPE")

# We need to include qtprivate api's
# As QAndroidBinder is not yet implemented with a public api
set(LIBS ${LIBS} Qt6::CorePrivate -ljnigraphics)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/platforms/android)

set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/android_vpnprotocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/installedAppsImageProvider.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/android_vpnprotocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/installedAppsImageProvider.cpp
)

foreach(abi IN ITEMS ${QT_ANDROID_ABIS})
    set_property(TARGET ${PROJECT} PROPERTY QT_ANDROID_EXTRA_LIBS
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/amneziawg/android/${abi}/libwg-go.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/android/${abi}/libck-ovpn-plugin.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/android/${abi}/libovpn3.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/android/${abi}/libovpnutil.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/android/${abi}/librsapss.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openssl/android/${abi}/libcrypto_3.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openssl/android/${abi}/libssl_3.so
        ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/libssh/android/${abi}/libssh.so
    )
endforeach()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/xray/android/libxray.aar
        DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/android/xray/libXray)
