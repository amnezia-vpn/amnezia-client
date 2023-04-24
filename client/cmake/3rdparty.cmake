set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules;${CMAKE_MODULE_PATH}")

if(NOT IOS AND NOT ANDROID)
   include(${CLIENT_ROOT_DIR}/3rd/SingleApplication/singleapplication.cmake)
endif()

add_subdirectory(${CLIENT_ROOT_DIR}/3rd/SortFilterProxyModel)
set(LIBS ${LIBS} SortFilterProxyModel)

include(${CLIENT_ROOT_DIR}/3rd/qrcodegen/qrcodegen.cmake)
include(${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/QSimpleCrypto.cmake)

set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
add_subdirectory(${CLIENT_ROOT_DIR}/3rd/zlib)
if(WIN32)
    set(ZLIB_LIBRARY $<IF:$<CONFIG:Debug>,zlibd,zlib>)
else()
    set(ZLIB_LIBRARY z)
endif()
set(ZLIB_INCLUDE_DIR "${CLIENT_ROOT_DIR}/3rd/zlib" "${CMAKE_CURRENT_BINARY_DIR}/3rd/zlib")
link_directories(${CMAKE_CURRENT_BINARY_DIR}/3rd/zlib)
link_libraries(${ZLIB_LIBRARY})

if(IOS)
    set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CLIENT_ROOT_DIR}/3rd/mbedtls)
    set(WITH_MBEDTLS ON CACHE BOOL "" FORCE)
    set(WITH_GCRYPT OFF CACHE BOOL "" FORCE)
    set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(HAVE_LIBCRYPTO OFF CACHE BOOL "" FORCE)
    set(MBEDTLS_ROOT_DIR "${CMAKE_CURRENT_BINARY_DIR}/3rd/mbedtls" CACHE PATH "" FORCE)
    set(MBEDTLS_INCLUDE_DIR "${CLIENT_ROOT_DIR}/3rd/mbedtls/include" CACHE PATH "" FORCE)
    set(MBEDTLS_LIBRARIES "mbedtls" "mbedx509" "mbedcrypto" CACHE STRING "" FORCE)
    set(MBEDTLS_FOUND TRUE CACHE BOOL "" FORCE)
    set(MBEDTLS_CRYPTO_LIBRARY "mbedcrypto" CACHE STRING "" FORCE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DMBEDTLS_ALLOW_PRIVATE_ACCESS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMBEDTLS_ALLOW_PRIVATE_ACCESS")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(WITH_STATIC_LIB ON CACHE BOOL "" FORCE)
    set(WITH_SYMBOL_VERSIONING OFF CACHE BOOL "" FORCE)

    include_directories(${CLIENT_ROOT_DIR}/3rd/mbedtls/include)
else(IOS)
    if(NOT LINUX)
        set(OPENSSL_ROOT_DIR "${CMAKE_CURRENT_BINARY_DIR}/3rd/OpenSSL")
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
        set(OPENSSL_LIBRARIES_DIR "${OPENSSL_ROOT_DIR}/lib")
        set(OPENSSL_LIBRARIES "ssl" "crypto")

        set(OPENSSL_PATH "${CLIENT_ROOT_DIR}/3rd/OpenSSL")
        if(WIN32)
            if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
                set(OPENSSL_LIB_SSL_PATH "${OPENSSL_PATH}/lib/windows/x86_64/libssl.lib")
                set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_PATH}/lib/windows/x86_64/libcrypto.lib")
            else()
                set(OPENSSL_LIB_SSL_PATH "${OPENSSL_PATH}/lib/windows/x86/libssl.lib")
                set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_PATH}/lib/windows/x86/libcrypto.lib")
            endif()
        elseif(APPLE AND NOT IOS)
            set(OPENSSL_LIB_SSL_PATH "${OPENSSL_PATH}/lib/macos/x86_64/libssl.a")
            set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_PATH}/lib/macos/x86_64/libcrypto.a")
        elseif(IOS)
            set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_LIBRARIES_DIR}/libcrypto.a")
            set(OPENSSL_SSL_LIBRARY "${OPENSSL_LIBRARIES_DIR}/libssl.a")
            set(OPENSSL_LIB_SSL_PATH "${OPENSSL_PATH}/lib/ios/iphone/libssl.a")
            set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_PATH}/lib/ios/iphone/libcrypto.a")
        elseif(ANDROID)
            set(abi ${CMAKE_ANDROID_ARCH_ABI})

            set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_LIBRARIES_DIR}/android/${abi}/libcrypto.a")
            set(OPENSSL_SSL_LIBRARY "${OPENSSL_LIBRARIES_DIR}/android/${abi}/libssl.a")
            set(OPENSSL_LIB_SSL_PATH "${OPENSSL_PATH}/lib/android/${abi}/libssl.a")
            set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_PATH}/lib/android/${abi}/libcrypto.a")

            set(OPENSSL_LIBRARIES_DIR "${OPENSSL_LIBRARIES_DIR}/android/${abi}")
        endif()

        file(COPY ${OPENSSL_LIB_SSL_PATH} ${OPENSSL_LIB_CRYPTO_PATH}
            DESTINATION ${OPENSSL_LIBRARIES_DIR})
        file(COPY "${OPENSSL_PATH}/include"
            DESTINATION ${OPENSSL_ROOT_DIR})
    endif()

    set(OPENSSL_USE_STATIC_LIBS TRUE)
    find_package(OpenSSL REQUIRED)
    set(LIBS ${LIBS}
        OpenSSL::Crypto
        OpenSSL::SSL
    )
endif(IOS)

set(WITH_GSSAPI OFF CACHE BOOL "" FORCE)
set(WITH_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CLIENT_ROOT_DIR}/3rd/libssh)
add_compile_definitions(_WINSOCKAPI_)
set(LIBS ${LIBS} ssh)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_QT6 ON)
add_subdirectory(${CLIENT_ROOT_DIR}/3rd/qtkeychain)
set(LIBS ${LIBS} qt6keychain)

include(${CLIENT_ROOT_DIR}/3rd/strongswan/strongswan.cmake)

include_directories(
    ${CLIENT_ROOT_DIR}/3rd/OpenSSL/include
    ${CLIENT_ROOT_DIR}/3rd/libssh/include
    ${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/include
    ${CLIENT_ROOT_DIR}/3rd/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/libssh/include
)
