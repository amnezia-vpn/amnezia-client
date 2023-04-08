cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT AmneziaVPN)
project(${PROJECT})

if(ANDROID)
    set(QT_ANDROID_BUILD_ALL_ABIS ON)
endif()

if(APPLE AND NOT IOS)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif()

add_subdirectory(client)

if(NOT IOS AND NOT ANDROID)
    add_subdirectory(service)
endif()
