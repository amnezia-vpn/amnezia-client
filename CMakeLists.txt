cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT AmneziaVPN)

project(${PROJECT} VERSION 2.1.2.0
        DESCRIPTION "AmneziaVPN"
        HOMEPAGE_URL "https://amnezia.org/"
)
set(RELEASE_DATE "09.03.2023")
set(APP_MAJOR_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})

if(ANDROID)
    set(QT_ANDROID_BUILD_ALL_ABIS ON)
endif()

add_subdirectory(client)

if(NOT IOS AND NOT ANDROID)
    add_subdirectory(service)

    include(${CMAKE_SOURCE_DIR}/deploy/installer/config.cmake)
endif()
