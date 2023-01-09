cmake_minimum_required(VERSION 3.23.0 FATAL_ERROR)

set(PROJECT AmneziaVPN)
project(${PROJECT})

#if(ANDROID)
#    set(QT_ANDROID_BUILD_ALL_ABIS ON)
#endif()

add_subdirectory(client)

if(NOT IOS AND NOT ANDROID)
    add_subdirectory(service)
endif()
