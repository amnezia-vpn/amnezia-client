cmake_minimum_required(VERSION 3.25.0)

project(AmneziaVPN_Tests)

find_package(Qt6 REQUIRED COMPONENTS Test)

list(REMOVE_ITEM SOURCES "${CLIENT_ROOT_DIR}/main.cpp")

add_executable(test_import_export
    testAdminSelfHostedExport.cpp
    mockRepositories.h
    ${SOURCES}
    ${HEADERS}
)

qt_add_repc_replicas(test_import_export 
    ${CLIENT_ROOT_DIR}/../ipc/ipc_interface.rep
    ${CLIENT_ROOT_DIR}/../ipc/ipc_process_interface.rep
    ${CLIENT_ROOT_DIR}/../ipc/ipc_process_tun2socks.rep
)

target_link_libraries(test_import_export PRIVATE
    Qt6::Test
    ${LIBS}
)

target_include_directories(test_import_export PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_BINARY_DIR}/..
)

enable_testing()
add_test(NAME ImportExportTest COMMAND test_import_export)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test_data)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/test_data/awg2-selfhosted.json
    ${CMAKE_CURRENT_BINARY_DIR}/test_data/awg2-selfhosted.json
    COPYONLY
)
