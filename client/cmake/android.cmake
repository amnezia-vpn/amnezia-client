message("Client android ${CMAKE_ANDROID_ARCH_ABI} build")

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

# libQt6Core is built against a recent LLVM libc++; if androiddeployqt packs an older
# libc++_shared.so (e.g. from NDK 25), dlopen fails on std::pmr::monotonic_buffer_resource.
# Pin the STL to the same NDK CMake uses (Qt 6.9.x expects NDK r27 — see Qt wiki).
if(CMAKE_ANDROID_NDK)
    file(GLOB _amnz_ndk_prebuilts
        LIST_DIRECTORIES true
        "${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/*")
    set(_amnz_ndk_prebuilt "")
    foreach(_amnz_d IN LISTS _amnz_ndk_prebuilts)
        if(IS_DIRECTORY "${_amnz_d}")
            set(_amnz_ndk_prebuilt "${_amnz_d}")
            break()
        endif()
    endforeach()
    if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
        set(_amnz_libcxx_triple "aarch64-linux-android")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
        set(_amnz_libcxx_triple "armv7a-linux-androideabi")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
        set(_amnz_libcxx_triple "i686-linux-android")
    elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
        set(_amnz_libcxx_triple "x86_64-linux-android")
    else()
        set(_amnz_libcxx_triple "")
    endif()
    if(_amnz_ndk_prebuilt AND _amnz_libcxx_triple)
        set(_amnz_libcxx_shared
            "${_amnz_ndk_prebuilt}/sysroot/usr/lib/${_amnz_libcxx_triple}/libc++_shared.so")
        if(EXISTS "${_amnz_libcxx_shared}")
            set_property(TARGET ${PROJECT} PROPERTY QT_ANDROID_EXTRA_LIBS "${_amnz_libcxx_shared}")
            message(STATUS "Android: QT_ANDROID_EXTRA_LIBS libc++_shared from NDK: ${_amnz_libcxx_shared}")
        else()
            message(WARNING "Android: libc++_shared not found at ${_amnz_libcxx_shared} (check CMAKE_ANDROID_NDK=${CMAKE_ANDROID_NDK})")
        endif()
    else()
        message(WARNING "Android: could not resolve NDK prebuilt under ${CMAKE_ANDROID_NDK}")
    endif()
endif()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/core/protocols/androidVpnProtocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/core/utils/installedAppsImageProvider.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/android_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/protocols/androidVpnProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/core/utils/installedAppsImageProvider.cpp
)


find_package(awg-android REQUIRED)
set(LIBS ${LIBS} amnezia::awg-android)
set_property(TARGET ${PROJECT} APPEND PROPERTY QT_ANDROID_EXTRA_LIBS ${AMNEZIA_ANDROID_LIBWG_PATH} ${AMNEZIA_ANDROID_LIBWG_QUICK_PATH})

find_package(amnezia-libxray REQUIRED)
file(COPY ${AMNEZIA_LIBXRAY_PATH} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/android/xray/libXray)

find_package(openvpn-pt-android REQUIRED)
set(LIBS ${LIBS} amnezia::openvpn-pt-android)
set_property(TARGET ${PROJECT} APPEND PROPERTY QT_ANDROID_EXTRA_LIBS ${OPENVPN_PT_ANDROID_LIBCK_OVPN_PLUGIN_PATH})
