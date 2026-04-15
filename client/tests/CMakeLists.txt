cmake_minimum_required(VERSION 3.25.0)
project(TransportTest)

set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

find_package(Qt6 REQUIRED COMPONENTS Core Network Test)

set(QSIMPLECRYPTO_DIR ${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/src)

set(OPENSSL_ROOT_DIR "${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/openssl/")
if(WIN32)
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/windows/include")
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        set(OPENSSL_LIB_SSL "${OPENSSL_ROOT_DIR}/windows/win64/libssl.lib")
        set(OPENSSL_LIB_CRYPTO "${OPENSSL_ROOT_DIR}/windows/win64/libcrypto.lib")
    else()
        set(OPENSSL_LIB_SSL "${OPENSSL_ROOT_DIR}/windows/win32/libssl.lib")
        set(OPENSSL_LIB_CRYPTO "${OPENSSL_ROOT_DIR}/windows/win32/libcrypto.lib")
    endif()
elseif(APPLE AND NOT IOS)
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/macos/include")
    set(OPENSSL_LIB_SSL "${OPENSSL_ROOT_DIR}/macos/lib/libssl.a")
    set(OPENSSL_LIB_CRYPTO "${OPENSSL_ROOT_DIR}/macos/lib/libcrypto.a")
elseif(LINUX)
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/linux/include")
    set(OPENSSL_LIB_SSL "${OPENSSL_ROOT_DIR}/linux/x86_64/libssl.a")
    set(OPENSSL_LIB_CRYPTO "${OPENSSL_ROOT_DIR}/linux/x86_64/libcrypto.a")
endif()

add_definitions(-DPROD_AGW_PUBLIC_KEY="$ENV{PROD_AGW_PUBLIC_KEY}")
add_definitions(-DDEV_AGW_PUBLIC_KEY="$ENV{DEV_AGW_PUBLIC_KEY}")

qt_add_executable(${PROJECT_NAME}
    tst_transports.cpp
    ${CLIENT_ROOT_DIR}/core/networkUtilities.cpp
    ${CLIENT_ROOT_DIR}/core/networkUtilities.h
    ${QSIMPLECRYPTO_DIR}/sources/QBlockCipher.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QRsa.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QX509.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QX509Store.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QAead.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CLIENT_ROOT_DIR}
    ${CLIENT_ROOT_DIR}/core
    ${QSIMPLECRYPTO_DIR}
    ${QSIMPLECRYPTO_DIR}/include
    ${OPENSSL_INCLUDE_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    CLIENT_SOURCE_DIR="${CLIENT_ROOT_DIR}"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Test
    ${OPENSSL_LIB_SSL}
    ${OPENSSL_LIB_CRYPTO}
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 crypt32)
endif()

# Copy gateway.json next to the test binary
if(EXISTS "${CLIENT_ROOT_DIR}/gateway.json")
    add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CLIENT_ROOT_DIR}/gateway.json"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endif()

add_test(NAME TransportTest COMMAND ${PROJECT_NAME})
