set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules;${CMAKE_MODULE_PATH}")

add_subdirectory(${CLIENT_ROOT_DIR}/3rd/SortFilterProxyModel)
set(LIBS ${LIBS} SortFilterProxyModel)
include(${CLIENT_ROOT_DIR}/cmake/QSimpleCrypto.cmake)

include(${CLIENT_ROOT_DIR}/3rd/qrcodegen/qrcodegen.cmake)

set(LIBSSH_ROOT_DIR "${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/libssh/")
set(OPENSSL_ROOT_DIR "${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/openssl/")

set(OPENSSL_LIBRARIES_DIR "${OPENSSL_ROOT_DIR}/lib")

if(WIN32)
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/windows/include")
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/windows/x86_64/ssh.lib")
        set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/windows/x86_64")
        set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/windows/win64/libssl.lib")
        set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/windows/win64/libcrypto.lib")
    else()
        set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/windows/x86/ssh.lib")
        set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/windows/x86")
        set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/windows/win32/libssl.lib")
        set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/windows/win32/libcrypto.lib")
    endif()
elseif(APPLE AND NOT IOS)
    if(MACOS_NE)
        set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/macos/universal2/libssh.a")
        set(ZLIB_LIB_PATH "${LIBSSH_ROOT_DIR}/macos/universal2/libz.a")
        set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/macos/universal2")
    else()
        set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/macos/x86_64/libssh.a")
        set(ZLIB_LIB_PATH "${LIBSSH_ROOT_DIR}/macos/x86_64/libz.a")
        set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/macos/x86_64")
    endif()
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/macos/include")
    set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/macos/lib/libssl.a")
    set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/macos/lib/libcrypto.a")
elseif(IOS)
    set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/ios/arm64")
    set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/ios/arm64/libssh.a")
    set(ZLIB_LIB_PATH "${LIBSSH_ROOT_DIR}/ios/arm64/libz.a")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/ios/iphone/include")
    set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/ios/iphone/lib/libssl.a")
    set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/ios/iphone/lib/libcrypto.a")
elseif(ANDROID)
    set(abi ${CMAKE_ANDROID_ARCH_ABI})
    set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/android/${abi}")
    set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/android/${abi}/libssh.so")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/android/include")
    set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/android/${abi}/libssl.a")
    set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/android/${abi}/libcrypto.a")
    set(OPENSSL_LIBRARIES_DIR "${OPENSSL_ROOT_DIR}/android/${abi}")
elseif(LINUX)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        # For ARM64, check if prebuilt libraries exist, otherwise use system libraries
        if(EXISTS "${LIBSSH_ROOT_DIR}/linux/aarch64/libssh.a")
            set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/linux/aarch64")
            set(ZLIB_LIB_PATH "${LIBSSH_ROOT_DIR}/linux/aarch64/libz.a")
            set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/linux/aarch64/libssh.a")
            set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/linux/include")
            set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/linux/aarch64/libssl.a")
            set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/linux/aarch64/libcrypto.a")
        else()
            # Use system libraries for ARM64
            message(STATUS "Using system OpenSSL and libssh for ARM64")
            find_package(OpenSSL REQUIRED)
            find_package(PkgConfig REQUIRED)
            pkg_check_modules(LIBSSH REQUIRED libssh)
            pkg_check_modules(ZLIB REQUIRED zlib)

            set(LIBSSH_INCLUDE_DIR ${LIBSSH_INCLUDE_DIRS})
            set(LIBSSH_LIB_PATH ${LIBSSH_LIBRARIES})
            set(ZLIB_LIB_PATH ${ZLIB_LIBRARIES})
            set(OPENSSL_INCLUDE_DIR ${OPENSSL_INCLUDE_DIR})
            set(OPENSSL_LIB_SSL_PATH ${OPENSSL_SSL_LIBRARY})
            set(OPENSSL_LIB_CRYPTO_PATH ${OPENSSL_CRYPTO_LIBRARY})
        endif()
    else()
        # x86_64
        set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/linux/x86_64")
        set(ZLIB_LIB_PATH "${LIBSSH_ROOT_DIR}/linux/x86_64/libz.a")
        set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/linux/x86_64/libssh.a")
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/linux/include")
        set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/linux/x86_64/libssl.a")
        set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/linux/x86_64/libcrypto.a")
    endif()
endif()

# Only copy static libraries if they exist (not for system libraries)
if(EXISTS "${OPENSSL_LIB_SSL_PATH}" AND EXISTS "${OPENSSL_LIB_CRYPTO_PATH}")
    file(COPY ${OPENSSL_LIB_SSL_PATH} ${OPENSSL_LIB_CRYPTO_PATH}
            DESTINATION ${OPENSSL_LIBRARIES_DIR})
endif()

set(OPENSSL_USE_STATIC_LIBS TRUE)
  
set(LIBS ${LIBS} 
    ${LIBSSH_LIB_PATH} 
    ${ZLIB_LIB_PATH}
)
  
set(LIBS ${LIBS}
    ${OPENSSL_LIB_SSL_PATH}
    ${OPENSSL_LIB_CRYPTO_PATH}
)

add_compile_definitions(_WINSOCKAPI_)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_QT6 ON)
add_subdirectory(${CLIENT_ROOT_DIR}/3rd/qtkeychain)
set(LIBS ${LIBS} qt6keychain)

include_directories(
    ${OPENSSL_INCLUDE_DIR}
    ${LIBSSH_INCLUDE_DIR}/include
    ${LIBSSH_ROOT_DIR}/include
    ${CLIENT_ROOT_DIR}/3rd/libssh/include
    ${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/src/include
    ${CLIENT_ROOT_DIR}/3rd/qtkeychain/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/libssh/include
)
