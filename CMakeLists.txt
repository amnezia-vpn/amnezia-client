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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE AND NOT IOS)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif()

if(NOT IOS AND NOT ANDROID)
    add_subdirectory(service)

    if (NOT APPLE)
        include(${CMAKE_SOURCE_DIR}/deploy/installer/config.cmake)
    endif()
endif()

add_subdirectory(client)
