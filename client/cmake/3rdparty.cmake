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
    set(OPENSSL_ROOT_DIR "${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/openssl/")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/ios/iphone/include")
else(IOS)
    set(OPENSSL_ROOT_DIR "${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/openssl/")
    set(OPENSSL_LIBRARIES "ssl" "crypto")
    set(OPENSSL_LIBRARIES_DIR "${OPENSSL_ROOT_DIR}/lib")

    if(WIN32)
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/windows/include")
        if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
            set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/windows/win64/libssl.lib")
            set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/windows/win64/libcrypto.lib")
        else()
            set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/windows/win32/libssl.lib")
            set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/windows/win32/libcrypto.lib")
        endif()
    elseif(APPLE AND NOT IOS)
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/macos/include")
        set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/macos/lib/libssl.a")
        set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/macos/lib/libcrypto.a")
    elseif(IOS)
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/ios/iphone/include")
        set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/ios/iphone/lib/libcrypto.a")
        set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/ios/iphone/lib/libssl.a")
        set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/ios/iphone/lib/libssl.a")
        set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/ios/iphone/lib/libcrypto.a")
    elseif(ANDROID)
        set(abi ${CMAKE_ANDROID_ARCH_ABI})
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/android/include")
        set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/android/${abi}/libcrypto.a")
        set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/android/${abi}/libssl.a")
        set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/android/${abi}/libssl.a")
        set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/android/${abi}/libcrypto.a")

        set(OPENSSL_LIBRARIES_DIR "${OPENSSL_ROOT_DIR}/android/${abi}")
    elseif(LINUX)
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/linux/include")
        set(OPENSSL_CRYPTO_LIBRARY "${OPENSSL_ROOT_DIR}/linux/${CMAKE_SYSTEM_PROCESSOR}/libcrypto.a")
        set(OPENSSL_SSL_LIBRARY "${OPENSSL_ROOT_DIR}/linux/${CMAKE_SYSTEM_PROCESSOR}/libssl.a")
        set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/linux/${CMAKE_SYSTEM_PROCESSOR}/libssl.a")
        set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/linux/${CMAKE_SYSTEM_PROCESSOR}/libcrypto.a")
    endif()
    
    file(COPY ${OPENSSL_LIB_SSL_PATH} ${OPENSSL_LIB_CRYPTO_PATH}
            DESTINATION ${OPENSSL_LIBRARIES_DIR})

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

if(IOS)
    set(LIBS ${LIBS}
            ${OPENSSL_ROOT_DIR}/ios/iphone/lib/libcrypto.a
            ${OPENSSL_ROOT_DIR}/ios/iphone/lib/libssl.a
    )
endif()

if(ANDROID)
    foreach(abi IN ITEMS ${QT_ANDROID_ABIS})
        if(CMAKE_ANDROID_ARCH_ABI STREQUAL ${abi})
            set(LIBS ${LIBS}
                ${OPENSSL_ROOT_DIR}/android/${abi}/libcrypto.a
                ${OPENSSL_ROOT_DIR}/android/${abi}/libssl.a
            )
        endif()
    endforeach()
endif()        

include_directories(
    ${OPENSSL_INCLUDE_DIR}
    ${CLIENT_ROOT_DIR}/3rd/libssh/include
    ${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/include
    ${CLIENT_ROOT_DIR}/3rd/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/libssh/include
)
