cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(A_PROJECT_BUILD_TYPE "Global")

set(PROJECT AmneziaVPN)

set(A_CMAKE_VERSION "${CMAKE_SOURCE_DIR}/Version.cmake")
if(EXISTS ${A_CMAKE_VERSION})
	include(${A_CMAKE_VERSION})
	message(STATUS "Included Version.cmake: ${A_CMAKE_VERSION}")
else()
	message(FATAL "No Version.cmake: ${A_CMAKE_VERSION}")
endif()

project(${PROJECT}
	VERSION ${A_PROJECT_VERSION}
	DESCRIPTION "AmneziaVPN"
	HOMEPAGE_URL "https://amnezia.org/"
)
set(RELEASE_DATE "2023-03-15")
set(APP_MAJOR_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})

if(ANDROID)
    set(QT_ANDROID_BUILD_ALL_ABIS ON)
endif()

if(APPLE AND NOT IOS)
    set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif()

add_subdirectory(client)

if(NOT IOS AND NOT ANDROID)
    add_subdirectory(service)

    include(${CMAKE_SOURCE_DIR}/deploy/installer/config.cmake)
endif()
