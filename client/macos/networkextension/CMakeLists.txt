enable_language(Swift)
message("Client message >> macos build >> networkextension")
set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

add_executable(networkextension)

configure_file(
    ${CMAKE_CURRENT_LIST_DIR}/Info.plist.in
    ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
)

if(MACOS_NE)
    message("MACOS_NE is ON")
    add_definitions(-DQ_OS_IOS)
    add_definitions(-DMACOS_NE)
    message("Add macros for MacOS Network Extension")
else()
    message("MACOS_NE is OFF")
endif()

set_target_properties(networkextension PROPERTIES
    XCODE_PRODUCT_TYPE com.apple.product-type.app-extension
    BUNDLE_EXTENSION appex

    #MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist

    MACOSX_BUNDLE_SHORT_VERSION_STRING "${APPLE_PROJECT_VERSION}"

    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}.network-extension"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_NAME "${BUILD_IOS_APP_IDENTIFIER}.network-extension"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${CMAKE_CURRENT_SOURCE_DIR}/AmneziaVPNNetworkExtension.entitlements
    XCODE_ATTRIBUTE_MARKETING_VERSION "${APP_MAJOR_VERSION}"
    XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${BUILD_ID}"
    XCODE_ATTRIBUTE_PRODUCT_NAME "AmneziaVPNNetworkExtension"

    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY "YES"
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"

    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../../Frameworks"

    XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
    # #XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual

    # #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution: Privacy Technologies OU (X7UJ388FXK)"
    # #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] "Apple Development"


    # #XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "Mac AppStore AmneziaVPN.network-extension"
    # #XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[variant=Debug] "Mac AppStore AmneziaVPN.network-extension"

    XCODE_ATTRIBUTE_INFOPLIST_FILE "${CMAKE_CURRENT_BINARY_DIR}/Info.plist"
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../../../../Frameworks @loader_path/../../../../Frameworks"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(networkextension PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(networkextension PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
        #XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual

        #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution: Privacy Technologies OU (X7UJ388FXK)"
        #XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] "Apple Development"


        #XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "Mac AppStore AmneziaVPN.network-extension"
        #XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[variant=Debug] "Mac AppStore AmneziaVPN.network-extension"
    )
endif()


set_target_properties(networkextension PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/WireGuardNetworkExtension-Bridging-Header.h"
    XCODE_ATTRIBUTE_SWIFT_OPTIMIZATION_LEVEL "-Onone"
    XCODE_ATTRIBUTE_SWIFT_PRECOMPILE_BRIDGING_HEADER "NO"
)

set_target_properties("networkextension" PROPERTIES
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "X7UJ388FXK"
)

find_library(FW_ASSETS_LIBRARY AssetsLibrary)
find_library(FW_MOBILE_CORE MobileCoreServices)
find_library(FW_UI_KIT UIKit)
find_library(FW_LIBRESOLV libresolv.9.tbd)

# set(OpenVPNAdapter_DIR "${CLIENT_ROOT_DIR}/3rd/")

# find_library(OPENVPN_ADAPTER_LIBRARY OpenVPNAdapter PATHS ${OpenVPNAdapter_DIR})
# target_link_libraries(networkextension PRIVATE ${OPENVPN_ADAPTER_LIBRARY})

# add_custom_command(TARGET networkextension PRE_BUILD
#     COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:networkextension>/../Frameworks
# )


# add_custom_command(TARGET networkextension POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E echo "Copying ${OPENVPN_ADAPTER_LIBRARY} to $<TARGET_FILE_DIR:networkextension>/../Frameworks/"
#     COMMAND ${CMAKE_COMMAND} -E copy_if_different
#     ${OPENVPN_ADAPTER_LIBRARY}
#     $<TARGET_FILE_DIR:networkextension>/../Frameworks/
#     COMMAND ${CMAKE_COMMAND} -E echo "Copy complete"
# )




# Set the root directory
set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

# Embedding the framework using CLIENT_ROOT_DIR
set_property(TARGET networkextension PROPERTY XCODE_EMBED_FRAMEWORKS
    "${CLIENT_ROOT_DIR}/3rd/OpenVPNAdapter/build/Release-macos/OpenVPNAdapter.framework"
)

# Setting the framework search paths using CLIENT_ROOT_DIR
set(CMAKE_XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS "${CLIENT_ROOT_DIR}/3rd/OpenVPNAdapter/build/Release-macos")

# Linking the framework using CLIENT_ROOT_DIR
target_link_libraries("networkextension" PRIVATE "${CLIENT_ROOT_DIR}/3rd/OpenVPNAdapter/build/Release-macos/OpenVPNAdapter.framework")



#target_link_libraries(networkextension PRIVATE ${FW_ASSETS_LIBRARY})
#target_link_libraries(networkextension PRIVATE ${FW_MOBILE_CORE})
#target_link_libraries(networkextension PRIVATE ${FW_UI_KIT})
target_link_libraries(networkextension PRIVATE ${FW_LIBRESOLV})

target_compile_options(networkextension PRIVATE -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\")
target_compile_options(networkextension PRIVATE -DNETWORK_EXTENSION=1)

set(WG_APPLE_SOURCE_DIR ${CLIENT_ROOT_DIR}/3rd/amneziawg-apple/Sources)

message("WG_APPLE_SOURCE_DIR is: ${WG_APPLE_SOURCE_DIR}")
message("CLIENT_ROOT_DIR is: ${CLIENT_ROOT_DIR}")

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
    WORKING_DIRECTORY ${CLIENT_ROOT_DIR}/3rd/wireguard-apple/Sources/WireGuardKitGo
    OUTPUT_VARIABLE WG_VERSION_FULL
)
string(REGEX REPLACE ".*v\([0-9.]*\).*" "\\1" WG_VERSION_STRING 1.1.1)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/wireguard-go-version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go-version.h)
target_sources(networkextension PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/wireguard-go-version.h)

target_include_directories(networkextension PRIVATE ${CLIENT_ROOT_DIR})
target_include_directories(networkextension PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(networkextension PRIVATE ${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/wireguard/macos/x86_64/libwg-go.a)

# Print the root directory for debugging purposes
message("---------")
message(${CLIENT_ROOT_DIR})
message(${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/xray/HevSocks5Tunnel.xcframework/macos-arm64_x86_64/libhev-socks5-tunnel.a)
target_link_libraries("networkextension" PRIVATE "${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/xray/HevSocks5Tunnel.xcframework/macos-arm64_x86_64/libhev-socks5-tunnel.a")

target_include_directories(networkextension PRIVATE ${CLIENT_ROOT_DIR}/3rd-prebuilt/3rd-prebuilt/xray/HevSocks5Tunnel.xcframework/macos-arm64_x86_64/Headers)