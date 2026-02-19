message("Client android ${CMAKE_ANDROID_ARCH_ABI} build")

# Option to build Play variant (with Google Play Billing) instead of OSS
# When ON, adds target android_play_apk: cmake --build . --target android_play_apk
option(ANDROID_BUILD_PLAY "Add android_play_apk target for Google Play Billing build" OFF)

set(APP_ANDROID_MIN_SDK 28)
set(ANDROID_PLATFORM "android-${APP_ANDROID_MIN_SDK}" CACHE STRING
    "The minimum API level supported by the application or library" FORCE)

# set QTP0002 policy: target properties that specify Android-specific paths may contain generator expressions
qt_policy(SET QTP0002 NEW)

set_target_properties(${PROJECT} PROPERTIES
    QT_ANDROID_VERSION_NAME ${CMAKE_PROJECT_VERSION}
    QT_ANDROID_VERSION_CODE ${APP_ANDROID_VERSION_CODE}
    QT_ANDROID_MIN_SDK_VERSION ${APP_ANDROID_MIN_SDK}
    QT_ANDROID_TARGET_SDK_VERSION 36
    QT_ANDROID_SDK_BUILD_TOOLS_REVISION 36.0.0
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)

set(QT_ANDROID_MULTI_ABI_FORWARD_VARS "QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL;CMAKE_BUILD_TYPE")

# We need to include qtprivate api's
# As QAndroidBinder is not yet implemented with a public api
# Check if Qt6::CorePrivate is available (may not be in all Qt versions/configurations)
if(TARGET Qt6::CorePrivate)
    set(LIBS ${LIBS} Qt6::CorePrivate)
endif()
set(LIBS ${LIBS} -ljnigraphics)

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

# Custom target to build Play variant (with Google Play Billing)
# Enable with: cmake -DANDROID_BUILD_PLAY=ON ...
# Then run: cmake --build <build_dir> --target android_play_apk
# Note: Do a normal build first so androiddeployqt creates the android-build folder
if(ANDROID_BUILD_PLAY)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_gradle_suffix "Debug")
    else()
        set(_gradle_suffix "Release")
    endif()
    set(_android_build_dir "${CMAKE_CURRENT_BINARY_DIR}/android-build-${PROJECT}")
    add_custom_target(android_play_apk
        COMMAND ./gradlew assemblePlay${_gradle_suffix} -DexplicitRun=1
        WORKING_DIRECTORY "${_android_build_dir}"
        COMMENT "Building Android Play variant (assemblePlay${_gradle_suffix})"
        DEPENDS ${PROJECT}
    )
endif()
