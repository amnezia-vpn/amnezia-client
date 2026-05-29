cmake_minimum_required(VERSION 3.25.0)

project(AmneziaVPN_Tests)

find_package(Qt6 REQUIRED COMPONENTS Test)


set(CMAKE_AUTORCC ON)

qt6_add_resources(TEST_QRC
    ${CLIENT_ROOT_DIR}/server_scripts/serverScripts.qrc
)

add_library(test_common OBJECT
    ${SOURCES}
    ${HEADERS}
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
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(test_import_export
    testAdminSelfHostedExport.cpp
)

target_link_libraries(test_import_export PRIVATE
    Qt6::Test
    test_common
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

add_executable(test_complex_operations
    testComplexOperations.cpp
)

target_link_libraries(test_complex_operations PRIVATE
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

add_executable(test_self_hosted_server_setup
    testSelfHostedServerSetup.cpp
)

target_link_libraries(test_self_hosted_server_setup PRIVATE
    Qt6::Test
    test_common
)

enable_testing()
add_test(NAME ImportExportTest COMMAND test_import_export)
add_test(NAME MultipleImportsTest COMMAND test_multiple_imports)
add_test(NAME ServerEditTest COMMAND test_server_edit)
add_test(NAME DefaultServerChangeTest COMMAND test_default_server_change)
add_test(NAME ServerEdgeCasesTest COMMAND test_server_edge_cases)
add_test(NAME SignalOrderTest COMMAND test_signal_order)
add_test(NAME ServersModelSyncTest COMMAND test_servers_model_sync)
add_test(NAME ComplexOperationsTest COMMAND test_complex_operations)
add_test(NAME SettingsSignalsTest COMMAND test_settings_signals)
add_test(NAME UiServersModelAndControllerTest COMMAND test_ui_servers_model_and_controller)
add_test(NAME SelfHostedServerSetupTest COMMAND test_self_hosted_server_setup)
