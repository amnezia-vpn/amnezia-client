cmake_minimum_required(VERSION 3.25)
project(SystemDnsTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
enable_testing()
find_package(Qt6 6.10 REQUIRED COMPONENTS Core Network Test)
set(ROOT "${CMAKE_CURRENT_LIST_DIR}/../..")
add_executable(systemdns_test
    tst_systemdns.cpp
    ${ROOT}/client/daemon/daemon.cpp
    ${ROOT}/client/daemon/daemon.h
    ${ROOT}/client/daemon/dnsutils.h
    ${ROOT}/client/daemon/wireguardutils.h
    ${ROOT}/client/daemon/interfaceconfig.cpp
    ${ROOT}/client/daemon/interfaceconfig.h
    ${ROOT}/client/mozilla/shared/ipaddress.cpp
)
# Replace logging only; the daemon, configuration parser and routing decisions
# under test are the production sources. No service or network changes are made.
target_include_directories(systemdns_test PRIVATE
    support ${ROOT}/client ${ROOT}/client/mozilla/shared)
target_link_libraries(systemdns_test PRIVATE Qt6::Core Qt6::Network Qt6::Test)
if(WIN32)
    target_sources(systemdns_test PRIVATE
        ${ROOT}/client/platforms/windows/daemon/dnsutilswindows.cpp
        ${ROOT}/client/platforms/windows/daemon/dnsutilswindows.h)
    target_compile_definitions(systemdns_test PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0A00)
    target_link_libraries(systemdns_test PRIVATE iphlpapi ws2_32)
elseif(APPLE)
    target_sources(systemdns_test PRIVATE
        ${ROOT}/client/platforms/macos/daemon/dnsutilsmacos.cpp
        ${ROOT}/client/platforms/macos/daemon/dnsutilsmacos.h)
    target_link_libraries(systemdns_test PRIVATE "-framework SystemConfiguration" "-framework CoreFoundation")
endif()
add_test(NAME systemdns COMMAND systemdns_test)
