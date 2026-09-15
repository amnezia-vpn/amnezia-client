cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

find_package(Qt6 REQUIRED COMPONENTS Core Test)
qt_standard_project_setup()

qt_add_executable(windows_route_capture_policy_tests
    windows_route_capture_policy_tests.cpp
)

target_include_directories(windows_route_capture_policy_tests PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../client/platforms/windows/daemon
)

target_link_libraries(windows_route_capture_policy_tests PRIVATE
    Qt6::Core
    Qt6::Test
    iphlpapi
    ws2_32
)

add_test(
    NAME windows_route_capture_policy_tests
    COMMAND windows_route_capture_policy_tests
)
