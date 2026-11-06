cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(WireguardAddressTests LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    find_package(Qt6 6.10 REQUIRED COMPONENTS Core Network Test)
    enable_testing()
endif()

set(CMAKE_AUTOMOC ON)
add_executable(test_wireguard_addresses testWireguardAddresses.cpp)
target_link_libraries(test_wireguard_addresses PRIVATE Qt6::Core Qt6::Network Qt6::Test)
add_test(NAME WireguardAddressesTest COMMAND test_wireguard_addresses)
