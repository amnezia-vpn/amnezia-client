cmake_minimum_required(VERSION 3.25.0)

project(AmneziaVPN_Tests)

find_package(Qt6 REQUIRED COMPONENTS Test)


set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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

add_executable(test_self_hosted_server_setup
    testSelfHostedServerSetup.cpp
)

target_link_libraries(test_self_hosted_server_setup PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_exports
    testMultipleExports.cpp
)

target_link_libraries(test_exports PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_serialization
    testSerialization.cpp
)

target_link_libraries(test_serialization PRIVATE
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

add_executable(test_ui_sites_model_and_controller
    testUiSitesModelAndController.cpp
)

target_link_libraries(test_ui_sites_model_and_controller PRIVATE
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

add_executable(test_ui_api_services_model_and_controller
    api/testUiApiServicesModelAndController.cpp
)

target_link_libraries(test_ui_api_services_model_and_controller PRIVATE
    Qt6::Test
    test_common
)

add_executable(test_ui_news_model_and_controller
    api/testUiNewsModelAndController.cpp
)

target_link_libraries(test_ui_news_model_and_controller PRIVATE
    Qt6::Test
    test_common
)

enable_testing()
add_test(NAME ImportExportTest COMMAND $<TARGET_FILE:test_import_export>)
add_test(NAME MultipleImportsTest COMMAND $<TARGET_FILE:test_multiple_imports>)
add_test(NAME ServerEditTest COMMAND $<TARGET_FILE:test_server_edit>)
add_test(NAME DefaultServerChangeTest COMMAND $<TARGET_FILE:test_default_server_change>)
add_test(NAME ServerEdgeCasesTest COMMAND $<TARGET_FILE:test_server_edge_cases>)
add_test(NAME SignalOrderTest COMMAND $<TARGET_FILE:test_signal_order>)
add_test(NAME ServersModelSyncTest COMMAND $<TARGET_FILE:test_servers_model_sync>)
add_test(NAME GatewayStacksTest COMMAND $<TARGET_FILE:test_gateway_stacks>)
add_test(NAME ComplexOperationsTest COMMAND $<TARGET_FILE:test_complex_operations>)
add_test(NAME SettingsSignalsTest COMMAND $<TARGET_FILE:test_settings_signals>)
add_test(NAME UiServersModelAndControllerTest COMMAND $<TARGET_FILE:test_ui_servers_model_and_controller>)
add_test(NAME SelfHostedServerSetupTest COMMAND $<TARGET_FILE:test_self_hosted_server_setup>)
add_test(NAME MultipleExportsTest COMMAND $<TARGET_FILE:test_exports>)
add_test(NAME SerializationTest COMMAND $<TARGET_FILE:test_serialization>)
add_test(NAME UiLanguageModelAndControllerTest COMMAND $<TARGET_FILE:test_ui_language_model_and_controller>)
add_test(NAME UiSitesModelAndControllerTest COMMAND $<TARGET_FILE:test_ui_sites_model_and_controller>)
add_test(NAME UiAppSTModelAndControllerTest COMMAND $<TARGET_FILE:test_ui_app_st_model_and_controller>)
add_test(NAME UiAllowedDnsModelAndControllerTest COMMAND $<TARGET_FILE:test_ui_allowed_dns_model_and_controller>)
add_test(NAME UiApiServicesModelAndControllerTest COMMAND $<TARGET_FILE:test_ui_api_services_model_and_controller>)
add_test(NAME UiNewsModelAndControllerTest COMMAND $<TARGET_FILE:test_ui_news_model_and_controller>)
