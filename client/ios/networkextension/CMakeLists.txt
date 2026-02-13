enable_language(Swift)

set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)
set(AMNEZIA_IOS_APPLETV ${AMNEZIA_IOS_ENABLE_APPLETV_TARGET})

if(AMNEZIA_IOS_APPLETV)
    message("Network Extension tvOS mode is ON")
else()
    message("Network Extension tvOS mode is OFF")
endif()

add_executable(networkextension)
set_target_properties(networkextension PROPERTIES
    XCODE_PRODUCT_TYPE com.apple.product-type.app-extension
    BUNDLE_EXTENSION appex

    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    MACOSX_BUNDLE_INFO_STRING "AmneziaVPNNetworkExtension"
    MACOSX_BUNDLE_BUNDLE_NAME "AmneziaVPNNetworkExtension"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}.network-extension"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${APPLE_PROJECT_VERSION}-${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${APPLE_PROJECT_VERSION}"

    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}.network-extension"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${CMAKE_CURRENT_SOURCE_DIR}/AmneziaVPNNetworkExtension.entitlements
    XCODE_ATTRIBUTE_MARKETING_VERSION "${APP_MAJOR_VERSION}"
    XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${BUILD_ID}"
    XCODE_ATTRIBUTE_PRODUCT_NAME "AmneziaVPNNetworkExtension"

    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY "YES"
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"

    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../../Frameworks"
)

if(AMNEZIA_IOS_APPLETV)
    set_target_properties(networkextension PROPERTIES
        XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "appletvos appletvsimulator"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "3"
        XCODE_ATTRIBUTE_TVOS_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}"
        XCODE_ATTRIBUTE_SDKROOT "appletvos"
        XCODE_ATTRIBUTE_SDKROOT[sdk=appletvos*] "appletvos"
        XCODE_ATTRIBUTE_SDKROOT[sdk=appletvsimulator*] "appletvsimulator"
        XCODE_ATTRIBUTE_LIBRARY_SEARCH_PATHS "$(inherited) $(SDKROOT)/usr/lib/swift $(TOOLCHAIN_DIR)/usr/lib/swift/$(PLATFORM_NAME)"
        XCODE_ATTRIBUTE_LIBRARY_SEARCH_PATHS[sdk=appletvos*] "$(inherited) $(SDKROOT)/usr/lib/swift $(TOOLCHAIN_DIR)/usr/lib/swift/$(PLATFORM_NAME)"
        XCODE_ATTRIBUTE_LIBRARY_SEARCH_PATHS[sdk=appletvsimulator*] "$(inherited) $(SDKROOT)/usr/lib/swift $(TOOLCHAIN_DIR)/usr/lib/swift/$(PLATFORM_NAME)"
        XCODE_ATTRIBUTE_EXCLUDED_LIBRARY_SEARCH_PATHS "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS*.sdk/usr/lib/swift"
        XCODE_ATTRIBUTE_EXCLUDED_FRAMEWORK_SEARCH_PATHS "/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS*.sdk/System/Library/Frameworks"
        LINKER_LANGUAGE Swift
    )
endif()

if(DEPLOY)
    set_target_properties(networkextension PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] "Apple Development"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "distr ios.org.amnezia.AmneziaVPN"
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[variant=Debug] "dev ios.org.amnezia.AmneziaVPN"
    )
else()
    set_target_properties(networkextension PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
    )
endif()

set_target_properties(networkextension PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
    XCODE_ATTRIBUTE_SWIFT_OPTIMIZATION_LEVEL "-Onone"
    XCODE_ATTRIBUTE_SWIFT_PRECOMPILE_BRIDGING_HEADER "NO"
)

if(NOT AMNEZIA_IOS_APPLETV)
    set_target_properties(networkextension PROPERTIES
        XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/WireGuardNetworkExtension-Bridging-Header.h"
    )
endif()

set_target_properties("networkextension" PROPERTIES
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "X7UJ388FXK"
)

find_library(FW_UI_KIT UIKit)
find_library(FW_LIBRESOLV libresolv.9.tbd)

if(NOT AMNEZIA_IOS_APPLETV)
    target_link_libraries(networkextension PRIVATE ${FW_UI_KIT})
    target_link_libraries(networkextension PRIVATE ${FW_LIBRESOLV})
endif()

target_compile_options(networkextension PRIVATE -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\")
target_compile_options(networkextension PRIVATE -DNETWORK_EXTENSION=1)

set(WG_APPLE_SOURCE_DIR ${CLIENT_ROOT_DIR}/3rd/amneziawg-apple/Sources)

set(NE_COMMON_SOURCES
    ${CLIENT_ROOT_DIR}/platforms/ios/NELogController.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/Log.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/LogRecord.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/PacketTunnelProvider.swift
)

set(NE_FULL_BACKEND_SOURCES
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/WireGuardAdapter.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/PacketTunnelSettingsGenerator.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/DNSResolver.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardNetworkExtension/ErrorNotifier.swift
    ${WG_APPLE_SOURCE_DIR}/Shared/Keychain.swift
    ${WG_APPLE_SOURCE_DIR}/Shared/FileManager+Extension.swift
    ${WG_APPLE_SOURCE_DIR}/Shared/Model/NETunnelProviderProtocol+Extension.swift
    ${WG_APPLE_SOURCE_DIR}/Shared/Model/TunnelConfiguration+WgQuickConfig.swift
    ${WG_APPLE_SOURCE_DIR}/Shared/Model/String+ArrayConversion.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/TunnelConfiguration.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/IPAddressRange.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/Endpoint.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/DNSServer.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/InterfaceConfiguration.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/PeerConfiguration.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKitC/x25519.c
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/Array+ConcurrentMap.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/IPAddress+AddrInfo.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKit/PrivateKey.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/HevSocksTunnel.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/PacketTunnelProvider+WireGuard.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/PacketTunnelProvider+OpenVPN.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/PacketTunnelProvider+Xray.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/WGConfig.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/XrayConfig.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/iosglue.mm
)

target_sources(networkextension PRIVATE ${NE_COMMON_SOURCES})

if(NOT AMNEZIA_IOS_APPLETV)
    target_sources(networkextension PRIVATE ${NE_FULL_BACKEND_SOURCES})
endif()

target_sources(networkextension PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/PrivacyInfo.xcprivacy
)

set_property(TARGET networkextension APPEND PROPERTY RESOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/PrivacyInfo.xcprivacy
)

if(NOT AMNEZIA_IOS_APPLETV)
    ## Build wireguard-go-version.h
    execute_process(
        COMMAND go list -m golang.zx2c4.com/wireguard
        WORKING_DIRECTORY ${CLIENT_ROOT_DIR}/3rd/wireguard-apple/Sources/WireGuardKitGo
        OUTPUT_VARIABLE WG_VERSION_FULL
    )
    string(REGEX REPLACE ".*v\([0-9.]*\).*" "\\1" WG_VERSION_STRING 1.1.1)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wireguard-go-version.h.in
                   ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go-version.h)
    target_sources(networkextension PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go-version.h)
endif()

target_include_directories(networkextension PRIVATE ${CLIENT_ROOT_DIR})
target_include_directories(networkextension PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if(NOT AMNEZIA_IOS_APPLETV)
    target_link_libraries(networkextension PRIVATE ${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/wireguard/ios/arm64/libwg-go.a)
    target_link_libraries(networkextension PRIVATE ${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/xray/HevSocks5Tunnel.xcframework)
endif()
