message("Client android ${CMAKE_ANDROID_ARCH_ABI} build")

if(NOT DEFINED APP_ANDROID_MIN_SDK)
    set(APP_ANDROID_MIN_SDK 28)
endif()

# Option to build Play variant (with Google Play Billing) instead of OSS
# When ON, adds target android_play_apk: cmake --build . --target android_play_apk
option(ANDROID_BUILD_PLAY "Add android_play_apk target for Google Play Billing build" OFF)
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

# libdnstt is built from the in-tree Go module in 3rd/dnstt using the NDK
# toolchain. It is deliberately not a Conan package: unlike libxray, the sources
# live in this repository, so there is no tarball to fetch and pin.
set(DNSTT_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd/dnstt)
set(DNSTT_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android/libs/${CMAKE_ANDROID_ARCH_ABI})
set(DNSTT_LIBRARY ${DNSTT_OUTPUT_DIR}/libdnstt.so)

if(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
    set(DNSTT_GOARCH arm64)
    set(DNSTT_CC_PREFIX aarch64-linux-android)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
    set(DNSTT_GOARCH arm)
    set(DNSTT_CC_PREFIX armv7a-linux-androideabi)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
    set(DNSTT_GOARCH amd64)
    set(DNSTT_CC_PREFIX x86_64-linux-android)
elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
    set(DNSTT_GOARCH 386)
    set(DNSTT_CC_PREFIX i686-linux-android)
endif()

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(DNSTT_NDK_HOST_TAG linux-x86_64)
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(DNSTT_NDK_HOST_TAG darwin-x86_64)
else()
    set(DNSTT_NDK_HOST_TAG windows-x86_64)
endif()

if(DEFINED CMAKE_ANDROID_NDK)
    set(DNSTT_NDK_ROOT ${CMAKE_ANDROID_NDK})
else()
    set(DNSTT_NDK_ROOT $ENV{ANDROID_NDK_ROOT})
endif()

find_program(GO_EXECUTABLE go)

if(GO_EXECUTABLE AND DNSTT_GOARCH AND DNSTT_NDK_ROOT)
    set(DNSTT_CC ${DNSTT_NDK_ROOT}/toolchains/llvm/prebuilt/${DNSTT_NDK_HOST_TAG}/bin/${DNSTT_CC_PREFIX}${APP_ANDROID_MIN_SDK}-clang)

    file(GLOB_RECURSE DNSTT_SOURCES CONFIGURE_DEPENDS
        ${DNSTT_MODULE_DIR}/*.go
        ${DNSTT_MODULE_DIR}/*.c
        ${DNSTT_MODULE_DIR}/*.h
    )

    add_custom_command(
        OUTPUT ${DNSTT_LIBRARY}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DNSTT_OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E env
            CGO_ENABLED=1
            GOOS=android
            GOARCH=${DNSTT_GOARCH}
            GOARM=7
            CC=${DNSTT_CC}
            ${GO_EXECUTABLE} build -buildmode=c-shared
            -ldflags "-s -w -extldflags=-Wl,-z,max-page-size=16384"
            -o ${DNSTT_LIBRARY} ./jni
        WORKING_DIRECTORY ${DNSTT_MODULE_DIR}
        DEPENDS ${DNSTT_SOURCES}
        COMMENT "Building libdnstt.so for ${CMAKE_ANDROID_ARCH_ABI}"
        VERBATIM
    )

    add_custom_target(libdnstt DEPENDS ${DNSTT_LIBRARY})
    add_dependencies(${PROJECT} libdnstt)
elseif(EXISTS ${DNSTT_LIBRARY})
    message(WARNING "Go toolchain not found; packaging the existing ${DNSTT_LIBRARY}")
else()
    message(WARNING "Go toolchain not found and no prebuilt libdnstt.so for ${CMAKE_ANDROID_ARCH_ABI}; "
                    "the DNSTT protocol will not work in this build")
endif()

find_package(openvpn-pt-android REQUIRED)
set(LIBS ${LIBS} amnezia::openvpn-pt-android)
set_property(TARGET ${PROJECT} APPEND PROPERTY QT_ANDROID_EXTRA_LIBS ${OPENVPN_PT_ANDROID_LIBCK_OVPN_PLUGIN_PATH})

set(APP_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android)

if(APP_ANDROID_MAX_SDK)
    set(APP_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/android-package-source)
    file(REMOVE_RECURSE ${APP_ANDROID_PACKAGE_SOURCE_DIR})
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/android/ DESTINATION ${APP_ANDROID_PACKAGE_SOURCE_DIR})

    set(manifest_path ${APP_ANDROID_PACKAGE_SOURCE_DIR}/AndroidManifest.xml)
    set(manifest_anchor "android:installLocation=\"auto\">")
    file(READ ${manifest_path} manifest_contents)
    string(REPLACE
        "${manifest_anchor}"
        "${manifest_anchor}\n\n    <uses-sdk android:maxSdkVersion=\"${APP_ANDROID_MAX_SDK}\" />"
        patched_contents "${manifest_contents}")
    if(patched_contents STREQUAL manifest_contents)
        message(FATAL_ERROR
            "Failed to set maxSdkVersion=${APP_ANDROID_MAX_SDK}: anchor '${manifest_anchor}' "
            "not found in ${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml")
    endif()
    file(WRITE ${manifest_path} "${patched_contents}")
endif()

set_property(TARGET ${PROJECT} PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR ${APP_ANDROID_PACKAGE_SOURCE_DIR})

if(QT_USE_TARGET_ANDROID_BUILD_DIR)
    set(_android_build_dir "${CMAKE_CURRENT_BINARY_DIR}/android-build-${PROJECT}")
else()
    set(_android_build_dir "${CMAKE_CURRENT_BINARY_DIR}/android-build")
endif()

add_custom_target(android_gradle_clean
    COMMAND ./gradlew clean
    WORKING_DIRECTORY "${_android_build_dir}"
    COMMENT "Cleaning Android Gradle build cache"
)

# Always-available debug target: build Play Debug APK and copy to standard output path
# so Qt Creator's deploy step picks it up automatically
add_custom_target(android_play_debug_install
    COMMAND ./gradlew assemblePlayDebug
    COMMAND sh -c "cp build/outputs/apk/play/debug/*.apk build/outputs/apk/android-build-${PROJECT}-debug.apk"
    WORKING_DIRECTORY "${_android_build_dir}"
    COMMENT "Building Android Play Debug APK and copying to deploy path"
    DEPENDS ${PROJECT}
)

if(ANDROID_BUILD_PLAY)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_gradle_suffix "Debug")
    else()
        set(_gradle_suffix "Release")
    endif()
    add_custom_target(android_play_apk
        COMMAND ./gradlew assemblePlay${_gradle_suffix}
        WORKING_DIRECTORY "${_android_build_dir}"
        COMMENT "Building Android Play APK (assemblePlay${_gradle_suffix})"
        DEPENDS ${PROJECT}
    )
    add_custom_target(android_play_aab
        COMMAND ./gradlew bundlePlay${_gradle_suffix}
        WORKING_DIRECTORY "${_android_build_dir}"
        COMMENT "Building Android Play AAB (bundlePlay${_gradle_suffix})"
        DEPENDS ${PROJECT}
    )
endif()
