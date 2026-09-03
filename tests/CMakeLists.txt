find_package(Qt6 REQUIRED COMPONENTS Core Network Test)

qt_add_executable(ip_split_tunneling_controller_test
    ipSplitTunnelingControllerTest.cpp
    ${CMAKE_SOURCE_DIR}/client/core/controllers/ipSplitTunnelingController.cpp
    ${CMAKE_SOURCE_DIR}/client/core/controllers/ipSplitTunnelingController.h
    ${CMAKE_SOURCE_DIR}/client/core/repositories/secureAppSettingsRepository.cpp
    ${CMAKE_SOURCE_DIR}/client/core/repositories/secureAppSettingsRepository.h
    ${CMAKE_SOURCE_DIR}/client/secureQSettings.cpp
    ${CMAKE_SOURCE_DIR}/client/secureQSettings.h
    ${CMAKE_SOURCE_DIR}/client/core/utils/networkUtilities.cpp
    ${CMAKE_SOURCE_DIR}/client/core/utils/networkUtilities.h
    ${CMAKE_SOURCE_DIR}/common/crypto/cryptoUtils.cpp
    ${CMAKE_SOURCE_DIR}/common/crypto/cryptoUtils.h
)

set_target_properties(ip_split_tunneling_controller_test PROPERTIES
    AUTOMOC ON
)

target_include_directories(ip_split_tunneling_controller_test PRIVATE
    ${CMAKE_SOURCE_DIR}/client
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/client
    ${CMAKE_BINARY_DIR}/3rd/qtkeychain
    ${CMAKE_SOURCE_DIR}/client/3rd/qtkeychain/qtkeychain
)

target_compile_definitions(ip_split_tunneling_controller_test PRIVATE
    CLIENT_KEYCHAIN_NAME_STR="AmneziaVPN-Test-Keychain"
    DEV_AGW_ENDPOINT=""
)

target_link_libraries(ip_split_tunneling_controller_test PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
    qt6keychain
    OpenSSL::Crypto
)

add_test(NAME ip_split_tunneling_controller_test
    COMMAND ip_split_tunneling_controller_test)
