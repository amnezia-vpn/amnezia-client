cmake_minimum_required(VERSION 3.25)
project(MacosDnsTests LANGUAGES CXX)
if(NOT APPLE)
    message(FATAL_ERROR "These tests require macOS SystemConfiguration headers")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Network Test)
enable_testing()
add_executable(test_macos_dns
    tst_macosdns.cpp
    ../../client/platforms/macos/daemon/dnsutilsmacos.cpp
    ../../client/platforms/macos/daemon/dnsutilsmacos.h
    ../../client/daemon/dnsutils.h
)
target_include_directories(test_macos_dns PRIVATE
    support ../../client ../../client/mozilla/shared)
target_link_libraries(test_macos_dns PRIVATE
    Qt6::Core Qt6::Network Qt6::Test
    "-framework SystemConfiguration" "-framework CoreFoundation")
add_test(NAME macos_dns COMMAND test_macos_dns)
# Redirect only the SystemConfiguration boundary; CoreFoundation and Qt are real.
foreach(api Create CopyKeyList CopyValue SetValue RemoveValue SetNotificationKeys SetDispatchQueue)
    target_compile_definitions(test_macos_dns PRIVATE
        SCDynamicStore${api}=FakeSCDynamicStore${api})
endforeach()

# Compile the same production source against the real API and production logger,
# without redirecting any symbols. This target performs no privileged operations.
add_library(macos_dns_native_compile OBJECT
    ../../client/platforms/macos/daemon/dnsutilsmacos.cpp
    ../../client/platforms/macos/daemon/dnsutilsmacos.h
    ../../client/daemon/dnsutils.h)
target_include_directories(macos_dns_native_compile PRIVATE
    ../../client ../../client/mozilla/shared ../../common/logger)
target_link_libraries(macos_dns_native_compile PRIVATE Qt6::Core Qt6::Network)

target_compile_definitions(test_macos_dns PRIVATE SCError=FakeSCError)
