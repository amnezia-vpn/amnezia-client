cmake_minimum_required(VERSION 3.25.0)

project(AmneziaVPN_Tests)

find_package(Qt6 REQUIRED COMPONENTS Test)

include(CTest)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

qt6_add_resources(TEST_QRC
    ${CLIENT_ROOT_DIR}/server_scripts/serverScripts.qrc
)

add_library(test_common OBJECT
    ${SOURCES}
    ${HEADERS}
    utils/testUtils.cpp
    ${TEST_QRC}
)

qt_add_repc_replicas(test_common
    ${CLIENT_ROOT_DIR}/../ipc/ipc_interface.rep
    ${CLIENT_ROOT_DIR}/../ipc/ipc_process_interface.rep
)

target_link_libraries(test_common PUBLIC
    ${LIBS}
)

target_include_directories(test_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_multiple_imports
    testMultipleImports.cpp
)

target_link_libraries(test_multiple_imports PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_server_edit
    testServerEdit.cpp
)

target_link_libraries(test_server_edit PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_default_server_change
    testDefaultServerChange.cpp
)

target_link_libraries(test_default_server_change PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_server_edge_cases
    testServerEdgeCases.cpp
)

target_link_libraries(test_server_edge_cases PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_signal_order
    testSignalOrder.cpp
)

target_link_libraries(test_signal_order PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_servers_model_sync
    testServersModelSync.cpp
)

target_link_libraries(test_servers_model_sync PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_server_rename
    testServerRename.cpp
)

target_link_libraries(test_server_rename PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_settings_signals
    testSettingsSignals.cpp
)

target_link_libraries(test_settings_signals PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_ui_servers_model_and_controller
    testUiServersModelAndController.cpp
)

target_link_libraries(test_ui_servers_model_and_controller PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_xray_serialization
    testXraySerialization.cpp
)

target_link_libraries(test_xray_serialization PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_ui_language_model_and_controller
    testUiLanguageModelAndController.cpp
)

target_link_libraries(test_ui_language_model_and_controller PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_ui_ip_model_and_controller
    testUiIpModelAndController.cpp
)

target_link_libraries(test_ui_ip_model_and_controller PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_ui_app_st_model_and_controller
    testUiAppSTModelAndController.cpp
)

target_link_libraries(test_ui_app_st_model_and_controller PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_ui_allowed_dns_model_and_controller
    testUiAllowedDnsModelAndController.cpp
)

target_link_libraries(test_ui_allowed_dns_model_and_controller PRIVATE
    Qt6::Test
    test_common
)

add_custom_command(TARGET test_xray_serialization POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vars.ini
        "$<TARGET_FILE_DIR:test_xray_serialization>"
)

add_custom_command(TARGET test_multiple_imports POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/test_vars.ini
        "$<TARGET_FILE_DIR:test_multiple_imports>"
)

add_subdirectory(selfHostedAdmin)
add_subdirectory(api)

enable_testing()
add_test(NAME MultipleImportsTest COMMAND test_multiple_imports)
add_test(NAME ServerEditTest COMMAND test_server_edit)
add_test(NAME DefaultServerChangeTest COMMAND test_default_server_change)
add_test(NAME ServerEdgeCasesTest COMMAND test_server_edge_cases)
add_test(NAME SignalOrderTest COMMAND test_signal_order)
add_test(NAME ServersModelSyncTest COMMAND test_servers_model_sync)
add_test(NAME ServerRenameTest COMMAND test_server_rename)
add_test(NAME SettingsSignalsTest COMMAND test_settings_signals)
add_test(NAME UiServersModelAndControllerTest COMMAND test_ui_servers_model_and_controller)
add_test(NAME XraySerializationTest COMMAND test_xray_serialization)
add_test(NAME UiLanguageModelAndControllerTest COMMAND test_ui_language_model_and_controller)
add_test(NAME UiIpModelAndControllerTest COMMAND test_ui_ip_model_and_controller)
add_test(NAME UiAppSTModelAndControllerTest COMMAND test_ui_app_st_model_and_controller)
add_test(NAME UiAllowedDnsModelAndControllerTest COMMAND test_ui_allowed_dns_model_and_controller)
