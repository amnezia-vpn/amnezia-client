cmake_minimum_required(VERSION 3.25)

# These tests can also run without building the application or installing a VPN
# service: cmake -S client/tests/recovery -B build-recovery -DCMAKE_PREFIX_PATH=...
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(ConnectionRecoveryTests LANGUAGES CXX)
    enable_testing()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Network Test Core5Compat)

get_filename_component(RECOVERY_CLIENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
if(TARGET test_common)
    set(RECOVERY_SUPPORT test_common)
else()
    set(RECOVERY_SUPPORT connection_recovery_support)
    set(CLIENT_APPLICATION_NAME "AmneziaVPN-recovery-tests")
    set(CLIENT_SERVICE_NAME "AmneziaVPN-recovery-tests-service")
    set(CLIENT_ORGANIZATION_NAME "AmneziaVPN-recovery-tests")
    configure_file("${RECOVERY_CLIENT_DIR}/../version.h.in" "version.h" @ONLY)

    add_library(connection_recovery_support STATIC
        "${RECOVERY_CLIENT_DIR}/mozilla/localsocketcontroller.cpp"
        "${RECOVERY_CLIENT_DIR}/mozilla/localsocketcontroller.h"
        "${RECOVERY_CLIENT_DIR}/mozilla/controllerimpl.h"
        "${RECOVERY_CLIENT_DIR}/../common/logger/logger.cpp"
        "${RECOVERY_CLIENT_DIR}/../common/logger/logger.h"
    )
    target_include_directories(connection_recovery_support PUBLIC
        "${RECOVERY_CLIENT_DIR}"
        "${RECOVERY_CLIENT_DIR}/mozilla/shared"
        "${RECOVERY_CLIENT_DIR}/../common/logger"
        "${CMAKE_CURRENT_BINARY_DIR}"
    )
    target_link_libraries(connection_recovery_support PUBLIC
        Qt6::Core Qt6::Network Qt6::Core5Compat
    )
    if(WIN32)
        target_compile_definitions(connection_recovery_support PRIVATE MZ_WINDOWS)
    endif()
endif()

add_executable(test_local_socket_recovery testLocalSocketRecovery.cpp)
target_link_libraries(test_local_socket_recovery PRIVATE ${RECOVERY_SUPPORT} Qt6::Test)
add_test(NAME LocalSocketRecoveryTest COMMAND test_local_socket_recovery)

add_executable(test_tunnel_pipe testTunnelPipe.cpp)
target_include_directories(test_tunnel_pipe PRIVATE "${RECOVERY_CLIENT_DIR}")
target_link_libraries(test_tunnel_pipe PRIVATE Qt6::Core Qt6::Network Qt6::Test)
add_test(NAME TunnelPipeTest COMMAND test_tunnel_pipe)
