cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT service)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT IOS AND NOT ANDROID AND NOT MACOS_NE)
    add_subdirectory(server)
endif()
