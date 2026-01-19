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

enable_testing()
add_test(NAME ImportExportTest COMMAND test_import_export)
add_test(NAME MultipleImportsTest COMMAND test_multiple_imports)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test_data/awg2-selfhosted.json
    ${CMAKE_CURRENT_BINARY_DIR}/test_data/awg2-selfhosted.json
    COPYONLY
)
