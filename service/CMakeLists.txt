cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT service)
project(${PROJECT} VERSION ${AMNEZIAVPN_VERSION})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT IOS AND NOT ANDROID)
    add_subdirectory(server)
endif()
