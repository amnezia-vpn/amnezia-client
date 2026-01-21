cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT AmneziaVPN)
set(AMNEZIAVPN_VERSION 4.8.12.8)

project(${PROJECT} VERSION ${AMNEZIAVPN_VERSION}
        DESCRIPTION "AmneziaVPN"
        HOMEPAGE_URL "https://amnezia.org/"
)

string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
set(RELEASE_DATE "${CURRENT_DATE}")

set(APP_MAJOR_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})
set(APP_ANDROID_VERSION_CODE 2104)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(MZ_PLATFORM_NAME "linux")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(MZ_PLATFORM_NAME "windows")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(MZ_PLATFORM_NAME "macos")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(MZ_PLATFORM_NAME "android")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    set(MZ_PLATFORM_NAME "ios")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(MZ_PLATFORM_NAME "wasm")
endif()

set(QT_BUILD_TOOLS_WHEN_CROSS_COMPILING ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    if(IOS)
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    elseif(MACOS_NE)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    else()
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
    endif()
endif()

add_subdirectory(client)

if(NOT IOS AND NOT ANDROID AND NOT MACOS_NE)
    add_subdirectory(service)

    include(${CMAKE_SOURCE_DIR}/deploy/installer/config.cmake)
endif()

set(AMNEZIA_STAGE_DIR "${CMAKE_BINARY_DIR}/stage")

if(WIN32 AND NOT IOS AND NOT ANDROID AND NOT MACOS_NE)
    file(TO_CMAKE_PATH "${AMNEZIA_STAGE_DIR}" AMNEZIA_STAGE_DIR_CMAKE)

    set(CPACK_GENERATOR "WIX")
    set(CPACK_WIX_VERSION 4)
    set(CPACK_PACKAGE_NAME "AmneziaVPN")
    set(CPACK_PACKAGE_VENDOR "AmneziaVPN")
    set(CPACK_PACKAGE_VERSION ${AMNEZIAVPN_VERSION})
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AmneziaVPN client")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "AmneziaVPN")
    set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}")
    set(CPACK_PACKAGE_EXECUTABLES "AmneziaVPN" "AmneziaVPN")
    set(CPACK_WIX_UPGRADE_GUID "{2D55AC62-96D6-4692-8C05-0D85BBF95485}")
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/client/images/app.ico")

    # WiX patches
    set(_AMNEZIA_WIX_PATCH_SERVICE "${CMAKE_SOURCE_DIR}/deploy/installer/wix/service_install_patch.xml")
    set(_AMNEZIA_WIX_PATCH_CLOSE_APP "${CMAKE_SOURCE_DIR}/deploy/installer/wix/close_client_patch.xml")
    file(TO_CMAKE_PATH "${_AMNEZIA_WIX_PATCH_SERVICE}" _AMNEZIA_WIX_PATCH_SERVICE_CMAKE)
    file(TO_CMAKE_PATH "${_AMNEZIA_WIX_PATCH_CLOSE_APP}" _AMNEZIA_WIX_PATCH_CLOSE_APP_CMAKE)
    set(CPACK_WIX_PATCH_FILE "${_AMNEZIA_WIX_PATCH_SERVICE_CMAKE};${_AMNEZIA_WIX_PATCH_CLOSE_APP_CMAKE}")

    # WiX v4 Util extension for CloseApplication + namespace for util
    set(CPACK_WIX_EXTENSIONS "${CPACK_WIX_EXTENSIONS};WixToolset.Util.wixext")
    set(CPACK_WIX_CUSTOM_XMLNS "util=http://wixtoolset.org/schemas/v4/wxs/util")

    set(CPACK_INSTALLED_DIRECTORIES "${AMNEZIA_STAGE_DIR_CMAKE};/")

    include(CPack)
endif()
