cmake_minimum_required(VERSION 3.25)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(AmneziaDomainRoutingTests LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    enable_testing()
endif()

find_package(Qt6 6.10 REQUIRED COMPONENTS Core Network Test)
set(CMAKE_AUTOMOC ON)

if(WIN32)
    find_package(Qt6 REQUIRED COMPONENTS Core5Compat)
    if(NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
        find_package(amnezia-xray-bindings REQUIRED)
    endif()
    if(TARGET amnezia::xray-bindings)
        set(_domain_test_xray amnezia::xray-bindings)
    elseif(AMNEZIA_TEST_XRAY_BINDINGS_DIR)
        add_library(domain_test_xray SHARED IMPORTED)
        set_target_properties(domain_test_xray PROPERTIES
            IMPORTED_IMPLIB "${AMNEZIA_TEST_XRAY_BINDINGS_DIR}/lib/amnezia_xray.lib"
            IMPORTED_LOCATION "${AMNEZIA_TEST_XRAY_BINDINGS_DIR}/bin/amnezia_xray.dll"
            INTERFACE_INCLUDE_DIRECTORIES "${AMNEZIA_TEST_XRAY_BINDINGS_DIR}/include"
        )
        set(_domain_test_xray domain_test_xray)
    endif()
    if(_domain_test_xray)
        add_executable(test_domain_routing_ownership testDomainRoutingOwnership.cpp ../../../service/server/xray.cpp)
        set_target_properties(test_domain_routing_ownership PROPERTIES AUTOMOC ON)
        target_include_directories(test_domain_routing_ownership PRIVATE ../.. ../../../service/server)
        target_link_libraries(test_domain_routing_ownership PRIVATE Qt6::Network Qt6::Core5Compat Qt6::Test ${_domain_test_xray} ws2_32)
        target_compile_options(test_domain_routing_ownership PRIVATE /utf-8)
        get_target_property(_domain_test_xray_type ${_domain_test_xray} TYPE)
        if(_domain_test_xray_type STREQUAL "SHARED_LIBRARY")
            add_custom_command(TARGET test_domain_routing_ownership POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${_domain_test_xray}>" "$<TARGET_FILE_DIR:test_domain_routing_ownership>"
            )
        endif()
        add_test(NAME DomainRoutingOwnershipTest COMMAND test_domain_routing_ownership)
    endif()
    add_executable(test_domain_route_monitor
        testDomainRouteMonitor.cpp
        ../../platforms/windows/daemon/windowsroutemonitor.h
        ../../mozilla/shared/ipaddress.cpp
    )
    target_include_directories(test_domain_route_monitor PRIVATE ../.. ../../mozilla ../../mozilla/shared ../../../common/logger)
    target_link_libraries(test_domain_route_monitor PRIVATE Qt6::Network Qt6::Test iphlpapi ws2_32)
    target_compile_options(test_domain_route_monitor PRIVATE /utf-8)
    add_test(NAME DomainRouteMonitorTest COMMAND test_domain_route_monitor)
endif()

add_executable(test_domain_routing
    testDomainRouting.cpp
    ../../core/utils/domainRouting.cpp
    ../../core/utils/domainRouting.h
)
target_include_directories(test_domain_routing PRIVATE ../..)
target_link_libraries(test_domain_routing PRIVATE Qt6::Core Qt6::Network Qt6::Test)
if(MSVC)
    target_compile_options(test_domain_routing PRIVATE /utf-8)
endif()
add_test(NAME DomainRoutingTest COMMAND test_domain_routing)
if(_domain_test_xray_type STREQUAL "SHARED_LIBRARY")
    set_tests_properties(DomainRoutingTest PROPERTIES ENVIRONMENT "AMNEZIA_XRAY_LIBRARY=$<TARGET_FILE:${_domain_test_xray}>")
endif()

find_package(Qt6 6.10 REQUIRED COMPONENTS Quick)
if(NOT TARGET SortFilterProxyModel)
    add_subdirectory(../../3rd/SortFilterProxyModel ${CMAKE_CURRENT_BINARY_DIR}/SortFilterProxyModel)
endif()
add_executable(test_domain_routing_qml testDomainRoutingQml.cpp)
target_link_libraries(test_domain_routing_qml PRIVATE Qt6::Quick Qt6::Test SortFilterProxyModel)
target_compile_definitions(test_domain_routing_qml PRIVATE AMNEZIA_QML_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/../../ui/qml")
if(MSVC)
    target_compile_options(test_domain_routing_qml PRIVATE /utf-8)
endif()
add_test(NAME DomainRoutingQmlTest COMMAND test_domain_routing_qml)
set_tests_properties(DomainRoutingQmlTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_CONTROLS_STYLE=Basic")
qt_add_resources(test_domain_routing_qml domain_routing_test_icons
    PREFIX "/images"
    BASE ../../images
    FILES ../../images/controls/plus.svg ../../images/controls/arrow-left.svg ../../images/controls/more-vertical.svg
)

if(WIN32 AND CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    find_package(Qt6 REQUIRED COMPONENTS RemoteObjects Core5Compat)
    add_library(domain_routing_protocol_compile OBJECT
        ../../core/protocols/domainRoutingProtocol.cpp
        ../../core/protocols/domainRoutingProtocol.h
    )
    qt_add_repc_replicas(domain_routing_protocol_compile ../../../ipc/ipc_interface.rep ../../../ipc/ipc_process_interface.rep)
    target_include_directories(domain_routing_protocol_compile PRIVATE ../.. ../../mozilla/shared ../../../ipc ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(domain_routing_protocol_compile PRIVATE Qt6::Network Qt6::Qml Qt6::RemoteObjects Qt6::Core5Compat)
    target_compile_options(domain_routing_protocol_compile PRIVATE /utf-8)
endif()
