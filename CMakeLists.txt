cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT AmneziaVPN)
if(NOT AMNEZIAVPN_VERSION)
    set(AMNEZIAVPN_VERSION 5.1.0.4 CACHE STRING "Client app version")
endif()

set(QT_CREATOR_SKIP_PACKAGE_MANAGER_SETUP ON CACHE BOOL "" FORCE)
include(${CMAKE_SOURCE_DIR}/cmake/cmake-conan/conan_bootstrap.cmake)
set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES
    ${CMAKE_SOURCE_DIR}/cmake/platform_settings.cmake
    ${CMAKE_SOURCE_DIR}/cmake/cmake-conan/conan_provider.cmake
    CACHE STRING "" FORCE)

project(${PROJECT} VERSION ${AMNEZIAVPN_VERSION}
        DESCRIPTION "AmneziaVPN"
        HOMEPAGE_URL "https://amnezia.org/"
)

option(AMNEZIA_BUILD_TESTS "Build test executables" OFF)

# trigger conan to kick off `conan install` globally
find_package(OpenSSL REQUIRED)
if (PREBUILTS_ONLY)
    return()
endif()

string(TIMESTAMP CURRENT_DATE "%Y-%m-%d")
set(RELEASE_DATE "${CURRENT_DATE}")

set(APP_MAJOR_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})

# bump by 2 on every release, because we're releasing two versions on the play store
set(APP_ANDROID_VERSION_CODE 2186)
if(DEFINED APP_ANDROID_VERSION_CODE_OFFSET)
    math(EXPR APP_ANDROID_VERSION_CODE "${APP_ANDROID_VERSION_CODE} + ${APP_ANDROID_VERSION_CODE_OFFSET}")
endif()

set(QT_BUILD_TOOLS_WHEN_CROSS_COMPILING ON)

add_subdirectory(client)

if(NOT IOS AND NOT ANDROID AND NOT MACOS_NE)
    add_subdirectory(service)
endif()

if ((LINUX AND NOT ANDROID) OR (APPLE AND NOT IOS AND NOT MACOS_NE) OR (WIN32))
    include(${CMAKE_SOURCE_DIR}/cmake/CPack.cmake)
endif()
