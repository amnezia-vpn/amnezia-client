find_package(Qt6 REQUIRED COMPONENTS Core Network Test)

qt_add_executable(ipv4interfaceaddress_test
    ipv4interfaceaddress_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../client/platforms/linux/daemon/ipv4interfaceaddress.cpp
)

target_include_directories(ipv4interfaceaddress_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../client/platforms/linux/daemon
)

target_link_libraries(ipv4interfaceaddress_test PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
)

add_test(NAME ipv4interfaceaddress_test COMMAND ipv4interfaceaddress_test)
