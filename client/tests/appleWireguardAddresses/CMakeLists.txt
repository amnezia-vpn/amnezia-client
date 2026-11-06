cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(AppleWireguardAddressTests LANGUAGES NONE)
    enable_testing()
endif()

find_program(SWIFTC_EXECUTABLE swiftc REQUIRED)
set(APPLE_WIREGUARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../3rd/amneziawg-apple")
set(APPLE_ADDRESS_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/testWGConfig.swift"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../platforms/ios/WGConfig.swift"
    "${APPLE_WIREGUARD_ROOT}/Sources/WireGuardKit/Endpoint.swift"
)
set(APPLE_ADDRESS_TEST_BINARY "${CMAKE_CURRENT_BINARY_DIR}/apple_wireguard_addresses_test")
add_custom_command(OUTPUT "${APPLE_ADDRESS_TEST_BINARY}"
    COMMAND "${SWIFTC_EXECUTABLE}" ${APPLE_ADDRESS_TEST_SOURCES} -o "${APPLE_ADDRESS_TEST_BINARY}"
    DEPENDS ${APPLE_ADDRESS_TEST_SOURCES}
    VERBATIM
)
add_custom_target(test_apple_wireguard_addresses ALL DEPENDS "${APPLE_ADDRESS_TEST_BINARY}")
add_test(NAME AppleWireguardAddressesTest COMMAND "${APPLE_ADDRESS_TEST_BINARY}")
