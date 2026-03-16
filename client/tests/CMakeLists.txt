cmake_minimum_required(VERSION 3.25.0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(AmneziaClientTests LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickTest Test)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_subdirectory(${CLIENT_ROOT_DIR}/3rd/SortFilterProxyModel ${CMAKE_CURRENT_BINARY_DIR}/SortFilterProxyModel)
endif()

add_executable(tst_scripts_registry
    core/tst_scripts_registry.cpp
    ${CLIENT_ROOT_DIR}/core/scripts_registry.cpp
    ${CLIENT_ROOT_DIR}/containers/containers_defs.cpp
    ${CLIENT_ROOT_DIR}/protocols/protocols_defs.cpp
    ${CLIENT_ROOT_DIR}/resources.qrc
)

target_include_directories(tst_scripts_registry PRIVATE
    ${CLIENT_ROOT_DIR}
)

target_link_libraries(tst_scripts_registry PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Test
)

add_executable(tst_client_qml
    qml/tst_client_qml.cpp
    ${CLIENT_ROOT_DIR}/resources.qrc
)

target_compile_definitions(tst_client_qml PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_LIST_DIR}/qml"
    CLIENT_QML_MODULES_DIR="${CLIENT_ROOT_DIR}/ui/qml/Modules"
)

target_include_directories(tst_client_qml PRIVATE
    ${CLIENT_ROOT_DIR}
)

target_link_libraries(tst_client_qml PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickTest
    SortFilterProxyModel
)

enable_testing()
add_test(NAME tst_scripts_registry COMMAND tst_scripts_registry)
add_test(NAME tst_client_qml COMMAND tst_client_qml)
set_tests_properties(tst_client_qml PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
