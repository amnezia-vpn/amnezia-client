set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules;${CMAKE_MODULE_PATH}")

if(NOT IOS AND NOT ANDROID)
   include(${CLIENT_ROOT_DIR}/3rd/SingleApplication/singleapplication.cmake)
endif()

add_subdirectory(${CLIENT_ROOT_DIR}/3rd/SortFilterProxyModel)
set(LIBS ${LIBS} SortFilterProxyModel)

include(${CLIENT_ROOT_DIR}/3rd/qrcodegen/qrcodegen.cmake)
include(${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/QSimpleCrypto.cmake)

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
      set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/macos/x86_64/libssh.a")
      set(ZLIB_LIB_PATH "${LIBSSH_ROOT_DIR}/macos/x86_64/libz.a")
      set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/macos/x86_64")
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
      set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/linux/x86_64")
      set(ZLIB_LIB_PATH "${LIBSSH_ROOT_DIR}/linux/x86_64/libz.a")
      set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/linux/x86_64/libssh.a")
      set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/linux/include")
      set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/linux/x86_64/libssl.a")
      set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/linux/x86_64/libcrypto.a")
  endif()
    
  file(COPY ${OPENSSL_LIB_SSL_PATH} ${OPENSSL_LIB_CRYPTO_PATH}
          DESTINATION ${OPENSSL_LIBRARIES_DIR})

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
    ${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/include
    ${CLIENT_ROOT_DIR}/3rd/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/libssh/include
)
