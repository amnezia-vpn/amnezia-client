cmake_minimum_required(VERSION 3.23.0 FATAL_ERROR)

set(PROJECT wireguard-service)
project(${PROJECT} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup()

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/wireguardtunnelservice.cpp
)

set(HEADERS
   ${CMAKE_CURRENT_LIST_DIR}/wireguardtunnelservice.h
)

set(LIBS
    user32
    rasapi32
    shlwapi
    iphlpapi
    ws2_32
    iphlpapi
    gdi32
    Advapi32
    Kernel32
)

add_executable(${PROJECT} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT} PRIVATE Qt6::Core ${LIBS})
