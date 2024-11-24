cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(CMAKE_PREFIX_PATH "/Users/kate/Qt/6.5.3/macos")

set(PROJECT ZloVPN)

file(READ project-version READ_VERSION_STRING)

string(REGEX MATCHALL "[^\n\r]+" READ_VERSION_LIST ${READ_VERSION_STRING})
list(GET READ_VERSION_LIST 0 READ_VERSION)
list(GET READ_VERSION_LIST 1 READ_VERSION_ANDROID)

message("Using version ${READ_VERSION}")
message("Using android version code ${READ_VERSION_ANDROID}")

project(${PROJECT} VERSION ${READ_VERSION}
        DESCRIPTION "ZloVPN"
        HOMEPAGE_URL "https://zlovpn.com/"
)

string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
set(RELEASE_DATE "${CURRENT_DATE}")

set(APP_MAJOR_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})
set(APP_ANDROID_VERSION_CODE ${READ_VERSION_ANDROID})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    set(MZ_PLATFORM_NAME "linux")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(MZ_PLATFORM_NAME "windows")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(MZ_PLATFORM_NAME "macos")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
    set(MZ_PLATFORM_NAME "android")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "iOS")
    set(MZ_PLATFORM_NAME "ios")
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Emscripten")
    set(MZ_PLATFORM_NAME "wasm")
endif ()

set(QT_BUILD_TOOLS_WHEN_CROSS_COMPILING ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (APPLE AND NOT IOS)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif ()


if (APPLE)
    cmake_policy(SET CMP0099 OLD)
    cmake_policy(SET CMP0114 NEW)

    set(APPLE_PROJECT_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})

    if (NOT BUILD_OSX_APP_IDENTIFIER)
        set(BUILD_OSX_APP_IDENTIFIER com.zloserver.vpn CACHE STRING "OSX Application identifier")
    endif ()
    if (NOT BUILD_OSX_SERVICE_IDENTIFIER)
        set(BUILD_OSX_SERVICE_IDENTIFIER com.zloserver.vpn-service CACHE STRING "OSX Service identifier")
    endif ()
    if (NOT BUILD_OSX_LAUNCHER_IDENTIFIER)
        set(BUILD_OSX_LAUNCHER_IDENTIFIER com.zloserver.vpn-service-launcher CACHE STRING "OSX Service launcher identifier")
    endif ()
    if (NOT BUILD_IOS_APP_IDENTIFIER)
        set(BUILD_IOS_APP_IDENTIFIER com.zloserver.vpn CACHE STRING "iOS Application identifier")
    endif ()
    if (NOT BUILD_IOS_GROUP_IDENTIFIER)
        set(BUILD_IOS_GROUP_IDENTIFIER group.com.zloserver.vpn CACHE STRING "iOS Group identifier")
    endif ()
    if (NOT BUILD_VPN_DEVELOPMENT_TEAM)
        set(BUILD_VPN_DEVELOPMENT_TEAM S6TBJMG52K CACHE STRING "iOS Development Team identifier")
    endif ()
    if (NOT BUILD_VPN_DEVELOPERID_TEAM)
        set(BUILD_VPN_DEVELOPERID_TEAM 8NJ4QZPB6B CACHE STRING "macOS Development Team identifier")
    endif ()

    set(CMAKE_XCODE_GENERATE_SCHEME FALSE)
    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${BUILD_VPN_DEVELOPMENT_TEAM})
    set(CMAKE_XCODE_ATTRIBUTE_GROUP_ID_IOS ${BUILD_IOS_GROUP_IDENTIFIER})

endif ()

if (NOT IOS AND NOT ANDROID)
    add_subdirectory(service)

    if (NOT APPLE)
        include(${CMAKE_SOURCE_DIR}/deploy/installer/config.cmake)
    endif ()
endif ()

add_subdirectory(client)
