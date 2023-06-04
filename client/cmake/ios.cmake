message("Client iOS build")
set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)
add_compile_definitions(MVPN_IOS)
set(APPLE_PROJECT_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})


enable_language(OBJC)
enable_language(OBJCXX)
enable_language(Swift)

find_package(Qt6 REQUIRED COMPONENTS ShaderTools)
set(LIBS ${LIBS} Qt6::ShaderTools)

find_library(FW_AUTHENTICATIONSERVICES AuthenticationServices)
find_library(FW_UIKIT UIKit)
find_library(FW_AVFOUNDATION AVFoundation)
find_library(FW_FOUNDATION Foundation)
find_library(FW_STOREKIT StoreKit)
find_library(FW_USERNOTIFICATIONS UserNotifications)

set(LIBS ${LIBS}
    ${FW_AUTHENTICATIONSERVICES} ${FW_UIKIT}
    ${FW_AVFOUNDATION} ${FW_FOUNDATION} ${FW_STOREKIT}
    ${FW_USERNOTIFICATIONS}
)


set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ios_vpnprotocol.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/iosnotificationhandler.h
    #${CMAKE_CURRENT_SOURCE_DIR}/mozilla/shared/bigint.h
    #${CMAKE_CURRENT_SOURCE_DIR}/mozilla/shared/bigintipv6addr.h
    ${CMAKE_CURRENT_SOURCE_DIR}/mozilla/shared/ipaddress.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/ipaddressrange.h   # TODO need refactor
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/QtAppDelegate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/QtAppDelegate-C-Interface.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ios_vpnprotocol.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/iosnotificationhandler.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/iosglue.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/mozilla/shared/ipaddress.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/ipaddressrange.cpp # TODO need refactor
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/QRCodeReaderBase.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/QtAppDelegate.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/MobileUtils.mm
)

#set(SOURCES ${SOURCES}
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/Shared/Keychain.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/IPAddressRange.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/InterfaceConfiguration.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/Shared/Model/NETunnelProviderProtocol+Extension.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/TunnelConfiguration.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/Shared/Model/TunnelConfiguration+WgQuickConfig.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/Endpoint.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/Shared/Model/String+ArrayConversion.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/PeerConfiguration.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/DNSServer.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardApp/LocalizationHelper.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/Shared/FileManager+Extension.swift
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKitC/x25519.c
#    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/PrivateKey.swift
#)

set(LIBS ${LIBS}
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/OpenSSL/lib/ios/iphone/libcrypto.a
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/OpenSSL/lib/ios/iphone/libssl.a
)

target_include_directories(${PROJECT} PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})


set_target_properties(${PROJECT} PROPERTIES
    XCODE_LINK_BUILD_PHASE_MODE KNOWN_LOCATION
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/Info.plist.in
    MACOSX_BUNDLE_ICON_FILE "AppIcon"
    MACOSX_BUNDLE_INFO_STRING "AmneziaVPN"
    MACOSX_BUNDLE_BUNDLE_NAME "AmneziaVPN"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${APPLE_PROJECT_VERSION}-${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${APPLE_PROJECT_VERSION}"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/ios/app/main.entitlements"
    XCODE_ATTRIBUTE_MARKETING_VERSION "${APPLE_PROJECT_VERSION}"
    XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    XCODE_ATTRIBUTE_PRODUCT_NAME "AmneziaVPN"
    XCODE_ATTRIBUTE_BUNDLE_INFO_STRING "AmneziaVPN"
    XCODE_GENERATE_SCHEME TRUE
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON
    XCODE_LINK_BUILD_PHASE_MODE KNOWN_LOCATION
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
    XCODE_EMBED_APP_EXTENSIONS networkextension
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
    XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] "Apple Development"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual
    XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "match AppStore org.amnezia.AmneziaVPN"
    XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[variant=Debug] "match Development org.amnezia.AmneziaVPN"
)
set_target_properties(${PROJECT} PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
    XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/WireGuard-Bridging-Header.h"
    XCODE_ATTRIBUTE_SWIFT_PRECOMPILE_BRIDGING_HEADER "NO"
    XCODE_ATTRIBUTE_SWIFT_OBJC_INTERFACE_HEADER_NAME "AmneziaVPN-Swift.h"
)
set_target_properties(${PROJECT} PROPERTIES
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "X7UJ388FXK"
)
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(${PROJECT} PRIVATE
    -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\"
    -DVPN_NE_BUNDLEID=\"${BUILD_IOS_APP_IDENTIFIER}.network-extension\"
)
target_sources(${PROJECT} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/iosvpnprotocol.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/ioslogger.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/Shared/Keychain.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/IPAddressRange.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/InterfaceConfiguration.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/Shared/Model/NETunnelProviderProtocol+Extension.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/TunnelConfiguration.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/Shared/Model/TunnelConfiguration+WgQuickConfig.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/Endpoint.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/Shared/Model/String+ArrayConversion.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/PeerConfiguration.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/DNSServer.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardApp/LocalizationHelper.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/Shared/FileManager+Extension.swift
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKitC/x25519.c
    ${CMAKE_CURRENT_SOURCE_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/PrivateKey.swift
)

target_sources(${PROJECT} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/launch.png
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/AmneziaVPNLaunchScreen.storyboard
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/Media.xcassets
)

set_property(TARGET ${PROJECT} APPEND PROPERTY RESOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/launch.png
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/AmneziaVPNLaunchScreen.storyboard
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/Media.xcassets
)

add_subdirectory(ios/networkextension)
add_dependencies(${PROJECT} networkextension)

set_property(TARGET ${PROJECT} PROPERTY XCODE_EMBED_FRAMEWORKS
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd/OpenVPNAdapter/build/Release-iphoneos/OpenVPNAdapter.framework"
)

set(CMAKE_XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/3rd/OpenVPNAdapter/build/Release-iphoneos)
target_link_libraries("networkextension" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rd/OpenVPNAdapter/build/Release-iphoneos/OpenVPNAdapter.framework")

