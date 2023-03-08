cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT AmneziaVPN)

set(BUILD_ID 1)
project(${PROJECT} VERSION 2.1.2 
        DESCRIPTION "AmneziaVPN"
        HOMEPAGE_URL "https://amnezia.org/"
)

if(ANDROID)
    set(QT_ANDROID_BUILD_ALL_ABIS ON)
endif()

add_subdirectory(client)

if(NOT IOS AND NOT ANDROID)
    add_subdirectory(service)
endif()
