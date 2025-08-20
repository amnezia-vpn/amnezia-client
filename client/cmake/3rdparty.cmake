set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/Modules;${CMAKE_MODULE_PATH}")

if(IOS)
    # On iOS (device and simulator), compile SortFilterProxyModel sources directly into the app target
    # to avoid Xcode OBJECT library linking issues.
    set(SFPM_DIR ${CLIENT_ROOT_DIR}/3rd/SortFilterProxyModel)
    target_sources(${PROJECT} PRIVATE
        ${SFPM_DIR}/qqmlsortfilterproxymodel.cpp
        ${SFPM_DIR}/filters/filter.cpp
        ${SFPM_DIR}/filters/filtercontainer.cpp
        ${SFPM_DIR}/filters/rolefilter.cpp
        ${SFPM_DIR}/filters/valuefilter.cpp
        ${SFPM_DIR}/filters/indexfilter.cpp
        ${SFPM_DIR}/filters/regexpfilter.cpp
        ${SFPM_DIR}/filters/rangefilter.cpp
        ${SFPM_DIR}/filters/expressionfilter.cpp
        ${SFPM_DIR}/filters/filtercontainerfilter.cpp
        ${SFPM_DIR}/filters/anyoffilter.cpp
        ${SFPM_DIR}/filters/alloffilter.cpp
        ${SFPM_DIR}/filters/filtersqmltypes.cpp
        ${SFPM_DIR}/sorters/sorter.cpp
        ${SFPM_DIR}/sorters/sortercontainer.cpp
        ${SFPM_DIR}/sorters/rolesorter.cpp
        ${SFPM_DIR}/sorters/stringsorter.cpp
        ${SFPM_DIR}/sorters/expressionsorter.cpp
        ${SFPM_DIR}/sorters/sortersqmltypes.cpp
        ${SFPM_DIR}/proxyroles/proxyrole.cpp
        ${SFPM_DIR}/proxyroles/proxyrolecontainer.cpp
        ${SFPM_DIR}/proxyroles/joinrole.cpp
        ${SFPM_DIR}/proxyroles/switchrole.cpp
        ${SFPM_DIR}/proxyroles/expressionrole.cpp
        ${SFPM_DIR}/proxyroles/proxyrolesqmltypes.cpp
        ${SFPM_DIR}/proxyroles/singlerole.cpp
        ${SFPM_DIR}/proxyroles/regexprole.cpp
        ${SFPM_DIR}/sorters/filtersorter.cpp
        ${SFPM_DIR}/proxyroles/filterrole.cpp
        ${SFPM_DIR}/utils/utils.cpp
    )
    target_include_directories(${PROJECT} PRIVATE ${SFPM_DIR})
else()
    add_subdirectory(${CLIENT_ROOT_DIR}/3rd/SortFilterProxyModel)
    set(LIBS ${LIBS} SortFilterProxyModel)
endif()



# Detect iOS simulator as early as possible for dependency selection
if(IOS AND CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
    set(IOS_SIM TRUE)
endif()

# Exclude QSimpleCrypto on iOS Simulator to avoid OpenSSL dependency
if(NOT (IOS AND IOS_SIM))
    include(${CLIENT_ROOT_DIR}/cmake/QSimpleCrypto.cmake)
endif()

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
    set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/macos/x86_64/libssh.a")
    set(ZLIB_LIB_PATH "${LIBSSH_ROOT_DIR}/macos/x86_64/libz.a")
    set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/macos/x86_64")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/macos/include")
    set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/macos/lib/libssl.a")
    set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/macos/lib/libcrypto.a")
elseif(IOS)
    # Detect simulator vs device SDK early (this file is included before ios.cmake)
    set(_MZ_IOS_SIMULATOR OFF)
    if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
        set(_MZ_IOS_SIMULATOR ON)
    endif()

    if(_MZ_IOS_SIMULATOR)
        # Expect simulator-slice prebuilts alongside device ones.
        # Suggested layout:
        #  - ${LIBSSH_ROOT_DIR}/ios/simulator/arm64/libssh.a
        #  - ${LIBSSH_ROOT_DIR}/ios/simulator/arm64/libz.a
        #  - ${OPENSSL_ROOT_DIR}/ios/simulator/lib/libssl.a
        #  - ${OPENSSL_ROOT_DIR}/ios/simulator/lib/libcrypto.a
        if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(_MZ_SIM_ARCH arm64)
        else()
            set(_MZ_SIM_ARCH x86_64)
        endif()

        set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/ios/simulator/${_MZ_SIM_ARCH}")
        set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/ios/simulator/${_MZ_SIM_ARCH}/libssh.a")
        set(ZLIB_LIB_PATH "${LIBSSH_ROOT_DIR}/ios/simulator/${_MZ_SIM_ARCH}/libz.a")
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/ios/simulator/include")
        set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/ios/simulator/lib/libssl.a")
        set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/ios/simulator/lib/libcrypto.a")

        # On iOS Simulator, we do not link OpenSSL/libssh/zlib at all; sources are stubbed.
    else()
        # Device (iphoneos) prebuilts
        set(LIBSSH_INCLUDE_DIR "${LIBSSH_ROOT_DIR}/ios/arm64")
        set(LIBSSH_LIB_PATH "${LIBSSH_ROOT_DIR}/ios/arm64/libssh.a")
        set(ZLIB_LIB_PATH "${LIBSSH_ROOT_DIR}/ios/arm64/libz.a")
        set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/ios/iphone/include")
        set(OPENSSL_LIB_SSL_PATH "${OPENSSL_ROOT_DIR}/ios/iphone/lib/libssl.a")
        set(OPENSSL_LIB_CRYPTO_PATH "${OPENSSL_ROOT_DIR}/ios/iphone/lib/libcrypto.a")
    endif()
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
    
# Only add and copy SSL/SSH/Zlib libraries when not building for iOS Simulator
if(NOT (IOS AND IOS_SIM))
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
endif()

add_compile_definitions(_WINSOCKAPI_)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_WITH_QT6 ON)
add_subdirectory(${CLIENT_ROOT_DIR}/3rd/qtkeychain)
set(LIBS ${LIBS} qt6keychain)

include_directories(
    $<$<NOT:$<AND:$<BOOL:${IOS}>,$<BOOL:${IOS_SIM}>>>:${OPENSSL_INCLUDE_DIR}>
    $<$<NOT:$<AND:$<BOOL:${IOS}>,$<BOOL:${IOS_SIM}>>>:${LIBSSH_INCLUDE_DIR}/include>
    $<$<NOT:$<AND:$<BOOL:${IOS}>,$<BOOL:${IOS_SIM}>>>:${LIBSSH_ROOT_DIR}/include>
    $<$<NOT:$<AND:$<BOOL:${IOS}>,$<BOOL:${IOS_SIM}>>>:${CLIENT_ROOT_DIR}/3rd/libssh/include>
    $<$<NOT:$<AND:$<BOOL:${IOS}>,$<BOOL:${IOS_SIM}>>>:${CLIENT_ROOT_DIR}/3rd/QSimpleCrypto/src/include>
    ${CLIENT_ROOT_DIR}/3rd/qtkeychain/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/qtkeychain
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/libssh/include
)
