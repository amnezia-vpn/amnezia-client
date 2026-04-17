enable_language(Swift)

set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

add_executable(networkextension)
set_target_properties(networkextension PROPERTIES
    LINKER_LANGUAGE Swift
    XCODE_PRODUCT_TYPE com.apple.product-type.app-extension
    BUNDLE_EXTENSION appex

    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    MACOSX_BUNDLE_INFO_STRING "FBLinkNetworkExtension"
    MACOSX_BUNDLE_BUNDLE_NAME "FBLinkNetworkExtension"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}.network-extension"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${APPLE_PROJECT_VERSION}-${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${APPLE_PROJECT_VERSION}"

    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}.network-extension"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${CMAKE_CURRENT_SOURCE_DIR}/FBLinkNetworkExtension.entitlements
    XCODE_ATTRIBUTE_MARKETING_VERSION "${APP_MAJOR_VERSION}"
    XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${BUILD_ID}"
    XCODE_ATTRIBUTE_PRODUCT_NAME "FBLinkNetworkExtension"

    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY "YES"
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos"
    XCODE_ATTRIBUTE_SUPPORTS_MACCATALYST "NO"
    XCODE_ATTRIBUTE_SUPPORTS_MAC_DESIGNED_FOR_IPHONE_IPAD "NO"

    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../../Frameworks"
    XCODE_ATTRIBUTE_LIBRARY_SEARCH_PATHS "$(inherited) $(TOOLCHAIN_DIR)/usr/lib/swift/$(PLATFORM_NAME) $(SDKROOT)/usr/lib/swift"
    XCODE_ATTRIBUTE_ALWAYS_EMBED_SWIFT_STANDARD_LIBRARIES "YES"
    XCODE_ATTRIBUTE_EMBEDDED_CONTENT_CONTAINS_SWIFT "YES"
    XCODE_ATTRIBUTE_OTHER_LDFLAGS "$(inherited) -ObjC"
)

if(DEPLOY)
    set_target_properties(networkextension PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] "Apple Development"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "distr ios.org.fblink.FBLink"
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[variant=Debug] "dev ios.org.fblink.FBLink"
    )
else()
    set_target_properties(networkextension PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED[variant=Debug] "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED[variant=Debug] "NO"
    )
endif()

set_target_properties(networkextension PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/WireGuardNetworkExtension-Bridging-Header.h"
    XCODE_ATTRIBUTE_SWIFT_OPTIMIZATION_LEVEL "-Onone"
    XCODE_ATTRIBUTE_SWIFT_PRECOMPILE_BRIDGING_HEADER "NO"
)

if(DEFINED BUILD_IOS_DEVELOPMENT_TEAM AND NOT BUILD_IOS_DEVELOPMENT_TEAM STREQUAL "")
    set_target_properties("networkextension" PROPERTIES
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${BUILD_IOS_DEVELOPMENT_TEAM}"
    )
endif()

target_link_options(networkextension PRIVATE
    "-framework AssetsLibrary"
    "-framework MobileCoreServices"
    "-framework UIKit"
    "-framework Foundation"
    "-framework Network"
    "-lresolv"
    "-lswiftCore"
    "-lswiftFoundation"
    "-lswiftDispatch"
    "-lswiftObjectiveC"
    "-Wl,-rpath,/usr/lib/swift"
)

target_compile_options(networkextension PRIVATE -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\")
target_compile_options(networkextension PRIVATE -DNETWORK_EXTENSION=1)

set(WG_APPLE_SOURCE_DIR ${CLIENT_ROOT_DIR}/3rd/amneziawg-apple/Sources)

target_sources(networkextension PRIVATE
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/WireGuardAdapter.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/PacketTunnelSettingsGenerator.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/DNSResolver.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardNetworkExtension/ErrorNotifier.swift
    ${WG_APPLE_SOURCE_DIR}/Shared/Keychain.swift
    ${WG_APPLE_SOURCE_DIR}/Shared/Model/TunnelConfiguration+WgQuickConfig.swift
    ${WG_APPLE_SOURCE_DIR}/Shared/Model/NETunnelProviderProtocol+Extension.swift
    ${WG_APPLE_SOURCE_DIR}/Shared/Model/String+ArrayConversion.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/TunnelConfiguration.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/IPAddressRange.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/Endpoint.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/DNSServer.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/InterfaceConfiguration.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/PeerConfiguration.swift
    ${WG_APPLE_SOURCE_DIR}/Shared/FileManager+Extension.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKitC/x25519.c
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/Array+ConcurrentMap.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/IPAddress+AddrInfo.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/PrivateKey.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/HevSocksTunnel.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/NELogController.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/Log.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/LogRecord.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/PacketTunnelProvider.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/PacketTunnelProvider+WireGuard.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/PacketTunnelProvider+OpenVPN.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/PacketTunnelProvider+Xray.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/WGConfig.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/XrayConfig.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/iosglue.mm
)

target_sources(networkextension PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/PrivacyInfo.xcprivacy
)

set_property(TARGET networkextension APPEND PROPERTY RESOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/PrivacyInfo.xcprivacy
)

## Build wireguard-go-version.h
execute_process(
    COMMAND go list -m golang.zx2c4.com/wireguard
    WORKING_DIRECTORY ${CLIENT_ROOT_DIR}/3rd/amneziawg-apple/Sources/WireGuardKitGo
    OUTPUT_VARIABLE WG_VERSION_FULL
)
string(REGEX REPLACE ".*v\([0-9.]*\).*" "\\1" WG_VERSION_STRING 1.1.1)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wireguard-go-version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go-version.h)
target_sources(networkextension PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go-version.h)

target_include_directories(networkextension PRIVATE ${CLIENT_ROOT_DIR})
target_include_directories(networkextension PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

set(WIREGUARD_IOS_LIB "${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/wireguard/ios/arm64/libwg-go.a")
if(NOT EXISTS "${WIREGUARD_IOS_LIB}")
    message(FATAL_ERROR "WireGuard iOS library is missing: ${WIREGUARD_IOS_LIB}")
endif()
target_link_libraries(networkextension PRIVATE "${WIREGUARD_IOS_LIB}")

set(HEV_XCFW_ROOT "${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/xray/HevSocks5Tunnel.xcframework")
set(HEV_IOS_SLICE "ios-arm64")
set(HEV_IOS_LIB "${HEV_XCFW_ROOT}/${HEV_IOS_SLICE}/libhev-socks5-tunnel.a")
set(HEV_IOS_HEADERS "${HEV_XCFW_ROOT}/${HEV_IOS_SLICE}/Headers")
if(NOT EXISTS "${HEV_IOS_LIB}")
    message(FATAL_ERROR "HevSocks5Tunnel iOS library is missing: ${HEV_IOS_LIB}")
endif()
target_link_libraries(networkextension PRIVATE "${HEV_IOS_LIB}")
target_include_directories(networkextension PRIVATE "${HEV_IOS_HEADERS}")
