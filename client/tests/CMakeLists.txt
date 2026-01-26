cmake_minimum_required(VERSION 3.25.0)

project(AmneziaVPN_Tests)

find_package(Qt6 REQUIRED COMPONENTS Test)

list(REMOVE_ITEM SOURCES "${CLIENT_ROOT_DIR}/main.cpp")

add_library(test_common OBJECT
    ${SOURCES}
    ${HEADERS}
)

qt_add_repc_replicas(test_common
    ${CLIENT_ROOT_DIR}/../ipc/ipc_interface.rep
    ${CLIENT_ROOT_DIR}/../ipc/ipc_process_interface.rep
    ${CLIENT_ROOT_DIR}/../ipc/ipc_process_tun2socks.rep
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

add_executable(test_gateway_stacks
    testGatewayStacks.cpp
)

target_link_libraries(test_gateway_stacks PRIVATE
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

enable_testing()
add_test(NAME ImportExportTest COMMAND test_import_export)
add_test(NAME MultipleImportsTest COMMAND test_multiple_imports)
add_test(NAME ServerEditTest COMMAND test_server_edit)
add_test(NAME DefaultServerChangeTest COMMAND test_default_server_change)
add_test(NAME ServerEdgeCasesTest COMMAND test_server_edge_cases)
add_test(NAME SignalOrderTest COMMAND test_signal_order)
add_test(NAME ServersModelSyncTest COMMAND test_servers_model_sync)
add_test(NAME GatewayStacksTest COMMAND test_gateway_stacks)
add_test(NAME ComplexOperationsTest COMMAND test_complex_operations)
add_test(NAME SettingsSignalsTest COMMAND test_settings_signals)
add_test(NAME UiServersModelAndControllerTest COMMAND test_ui_servers_model_and_controller)
