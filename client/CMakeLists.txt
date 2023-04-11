cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT AmneziaVPN)
project(${PROJECT} VERSION 2.1.2)
set(BUILD_ID 1)
SET(QT_BUILD_TOOLS_WHEN_CROSS_COMPILING ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.17)
    cmake_policy(SET CMP0099 OLD)
endif()

if(CMAKE_XCODE_BUILD_SYSTEM VERSION_GREATER_EQUAL 12)
  cmake_policy(SET CMP0114 NEW)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "Autogen")
set_property(GLOBAL PROPERTY AUTOMOC_TARGETS_FOLDER "Autogen")
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Autogen")

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets Core Gui Network Xml 
    RemoteObjects Quick Svg QuickControls2 
    Core5Compat Concurrent
)
set(LIBS ${LIBS} 
    Qt6::Widgets Qt6::Core Qt6::Gui
    Qt6::Network Qt6::Xml Qt6::RemoteObjects
    Qt6::Quick Qt6::Svg Qt6::QuickControls2
    Qt6::Core5Compat Qt6::Concurrent
)

qt_standard_project_setup()

if(IOS)
    execute_process(COMMAND bash ${CMAKE_CURRENT_LIST_DIR}/scripts/openvpn.sh args
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
endif()

set(IS_CI ${CI})
if(IS_CI)
  message("Detected CI env")
  find_program(CCACHE "ccache")
  if(CCACHE)
      set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE}")
  endif()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/3rdparty.cmake)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../ipc
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/migrations.h
    ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc.h
    ${CMAKE_CURRENT_LIST_DIR}/amnezia_application.h
    ${CMAKE_CURRENT_LIST_DIR}/containers/containers_defs.h
    ${CMAKE_CURRENT_LIST_DIR}/core/defs.h
    ${CMAKE_CURRENT_LIST_DIR}/core/errorstrings.h
    ${CMAKE_CURRENT_LIST_DIR}/core/scripts_registry.h
    ${CMAKE_CURRENT_LIST_DIR}/core/server_defs.h
    ${CMAKE_CURRENT_LIST_DIR}/core/servercontroller.h
    ${CMAKE_CURRENT_LIST_DIR}/platforms/linux/leakdetector.h
    ${CMAKE_CURRENT_LIST_DIR}/protocols/protocols_defs.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/notificationhandler.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/property_helper.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/uilogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/qautostart.h
    ${CMAKE_CURRENT_LIST_DIR}/protocols/vpnprotocol.h
)

if(NOT IOS)
    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/MobileUtils.h
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/QRCodeReaderBase.h
    )
endif()

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/migrations.cpp
    ${CMAKE_CURRENT_LIST_DIR}/amnezia_application.cpp
    ${CMAKE_CURRENT_LIST_DIR}/containers/containers_defs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/errorstrings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/scripts_registry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/server_defs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/servercontroller.cpp
    ${CMAKE_CURRENT_LIST_DIR}/platforms/linux/leakdetector.cpp
    ${CMAKE_CURRENT_LIST_DIR}/protocols/protocols_defs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/notificationhandler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/uilogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/qautostart.cpp
    ${CMAKE_CURRENT_LIST_DIR}/protocols/vpnprotocol.cpp
)

if(NOT IOS)
    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/MobileUtils.cpp
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/QRCodeReaderBase.cpp
    )
endif()

file(GLOB COMMON_FILES_H CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.h)
file(GLOB COMMON_FILES_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

file(GLOB_RECURSE PAGE_LOGIC_H CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/*.h)
file(GLOB_RECURSE PAGE_LOGIC_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/*.cpp)

file(GLOB CONFIGURATORS_H CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/configurators/*.h)
file(GLOB CONFIGURATORS_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/configurators/*.cpp)

file(GLOB UI_MODELS_H CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ui/models/*.h)
file(GLOB UI_MODELS_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ui/models/*.cpp)

set(HEADERS ${HEADERS}
    ${COMMON_FILES_H}
    ${PAGE_LOGIC_H}
    ${CONFIGURATORS_H}
    ${UI_MODELS_H}
)
set(SOURCES ${SOURCES}
    ${COMMON_FILES_CPP}
    ${PAGE_LOGIC_CPP}
    ${CONFIGURATORS_CPP}
    ${UI_MODELS_CPP}
)

qt6_add_resources(QRC ${QRC} ${CMAKE_CURRENT_LIST_DIR}/resources.qrc)

if(WIN32)
    add_compile_definitions(MVPN_WINDOWS)

    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/protocols/ikev2_vpn_protocol_windows.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/protocols/ikev2_vpn_protocol_windows.cpp
    )

    set(RESOURCES ${RESOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/platforms/windows/amneziavpn.rc
    )

    set(LIBS ${LIBS}
        user32
        rasapi32
        shlwapi
        iphlpapi
        ws2_32
        gdi32
    )

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        message("Windows x86_64 build")
        link_directories(${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/lib/windows/x86_64)
        set(LIBS ${LIBS}
            libssl
            libcrypto
        )
    else()
        message("Windows x86 build")
        link_directories(${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/lib/windows/x86)
        set(LIBS ${LIBS}
            libssl
            libcrypto
        )
    endif()
endif()

if(APPLE)
    if(NOT BUILD_OSX_APP_IDENTIFIER)
        set(BUILD_OSX_APP_IDENTIFIER org.amnezia.AmneziaVPN CACHE STRING "OSX Application identifier")
    endif()
    if(NOT BUILD_IOS_APP_IDENTIFIER)
        set(BUILD_IOS_APP_IDENTIFIER org.amnezia.AmneziaVPN CACHE STRING "iOS Application identifier")
    endif()
    if(NOT BUILD_IOS_GROUP_IDENTIFIER)
        set(BUILD_IOS_GROUP_IDENTIFIER group.org.amnezia.AmneziaVPN.Guardian CACHE STRING "iOS Group identifier")
    endif()
    if(NOT BUILD_VPN_DEVELOPMENT_TEAM)
        set(BUILD_VPN_DEVELOPMENT_TEAM X7UJ388FXK CACHE STRING "Amnezia VPN Development Team")
    endif()

    set(CMAKE_XCODE_GENERATE_SCHEME FALSE)
    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${BUILD_VPN_DEVELOPMENT_TEAM})
    set(CMAKE_XCODE_ATTRIBUTE_GROUP_ID_IOS ${BUILD_IOS_GROUP_IDENTIFIER})

   #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../build)
    if(NOT IOS)
        set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)

        message("MAC build")
        set(HEADERS ${HEADERS} ${CMAKE_CURRENT_LIST_DIR}/ui/macos_util.h)
        set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/ui/macos_util.mm)

        # set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14)
        add_compile_definitions(MVPN_MACOS)
        # ICON   = $$PWD/images/app.icns
        find_library(LIB_LIBCRYPTO NAMES "libcrypto.a"
            PATHS ${PROJECT_SOURCE_DIR}/3rd/OpenSSL/lib/macos/x86_64/ NO_DEFAULT_PATH)

        find_library(LIB_SSL NAMES "libssl.a"
            PATHS ${PROJECT_SOURCE_DIR}/3rd/OpenSSL/lib/macos/x86_64/ NO_DEFAULT_PATH)


        find_library(FW_COCOA Cocoa)
        find_library(FW_APPLICATIONSERVICES ApplicationServices)
        find_library(FW_CORESERVICES CoreServices)
        find_library(FW_FOUNDATION Foundation)
        find_library(FW_APPKIT AppKit)
        find_library(FW_SECURITY Security)

        set(LIBS ${LIBS} 
            ${FW_COCOA} ${FW_APPLICATIONSERVICES}
            ${FW_FOUNDATION} ${FW_APPKIT}
            ${FW_SECURITY} ${FW_CORESERVICES}
            ${LIB_LIBCRYPTO} ${LIB_SSL}
        )
    endif()
endif()


if(LINUX AND NOT ANDROID)
    add_compile_definitions(MVPN_LINUX)

    set(OPENSSL_USE_STATIC_LIBS TRUE)
    find_package(OpenSSL REQUIRED)
    set(LIBS ${LIBS}
        OpenSSL::Crypto
        OpenSSL::SSL
    )
    link_directories(${CMAKE_CURRENT_LIST_DIR}/platforms/linux)
endif()

if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    message("Client desktop build")
    add_compile_definitions(AMNEZIA_DESKTOP)
    
    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/core/ipcclient.h
        ${CMAKE_CURRENT_LIST_DIR}/core/privileged_process.h
        ${CMAKE_CURRENT_LIST_DIR}/ui/systemtray_notificationhandler.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/openvpnprotocol.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/openvpnovercloakprotocol.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/shadowsocksvpnprotocol.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/wireguardprotocol.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/core/ipcclient.cpp
        ${CMAKE_CURRENT_LIST_DIR}/core/privileged_process.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ui/systemtray_notificationhandler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/openvpnprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/openvpnovercloakprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/shadowsocksvpnprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/wireguardprotocol.cpp
    )
endif()

if(ANDROID)
    message("Client android ${CMAKE_ANDROID_ARCH_ABI} build")
    # We need to include qtprivate api's
    # As QAndroidBinder is not yet implemented with a public api
    set(LIBS ${LIBS} Qt6::CorePrivate)

    add_compile_definitions(MVPN_ANDROID)

    link_directories(${CMAKE_CURRENT_LIST_DIR}/platforms/android)

    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/android_controller.h
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/android_notificationhandler.h
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/androidutils.h
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/androidvpnactivity.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/android_vpnprotocol.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/android_controller.cpp
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/android_notificationhandler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/androidutils.cpp
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/androidvpnactivity.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/android_vpnprotocol.cpp
    )
endif()



if(IOS)
    message("Client iOS build")

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

    add_compile_definitions(MVPN_IOS)

    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/protocols/ios_vpnprotocol.h 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/iosnotificationhandler.h 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/json.h 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/bigint.h 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/bigintipv6addr.h 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/ipaddress.h 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/ipaddressrange.h 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/QtAppDelegate.h 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/QtAppDelegate-C-Interface.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/protocols/ios_vpnprotocol.mm 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/iosnotificationhandler.mm 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/json.cpp 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/iosglue.mm 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/ipaddress.cpp 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/ipaddressrange.cpp 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/QRCodeReaderBase.mm 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/QtAppDelegate.mm 
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/MobileUtils.mm
    )
endif()

if(CMAKE_OSX_SYSROOT STREQUAL "iphoneos")
    message("Building for iPhone OS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)

endif()

qt_add_executable(${PROJECT} ${SOURCES} ${HEADERS} ${RESOURCES} ${QRC})
qt_add_translations(${PROJECT} TS_FILES
    ${CMAKE_CURRENT_LIST_DIR}/translations/amneziavpn_ru.ts)

if(APPLE AND NOT IOS)
    set_target_properties(AmneziaVPN PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()

if(IOS)
    enable_language(OBJC)
    enable_language(OBJCXX)
    enable_language(Swift)

    #disable in cicd
    include(cmake/osxtools.cmake)
    # set(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY TRUE)

    set_target_properties(${PROJECT} PROPERTIES XCODE_ATTRIBUTE_ENABLE_BITCODE "NO")
    set_target_properties(${PROJECT} PROPERTIES XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")

    set_target_properties(${PROJECT} PROPERTIES XCODE_LINK_BUILD_PHASE_MODE KNOWN_LOCATION)
    set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../../Frameworks")
    set(CMAKE_XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS ${CMAKE_CURRENT_LIST_DIR}/3rd/OpenVPNAdapter/build/Release-iphoneos)


    #need to change for debug and release
    set_target_properties(${PROJECT} 
        PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.amnezia.${PROJECT}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "X7UJ388FXK"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
        )

    set_target_properties(${PROJECT}
        PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.amnezia.${PROJECT}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "X7UJ388FXK"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] "Apple Development"
        )

    set(LIBS ${LIBS}
        ${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/lib/ios/iphone/libcrypto.a
        ${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/lib/ios/iphone/libssl.a
        )

    target_include_directories(${PROJECT} PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})

    set_target_properties(${PROJECT} PROPERTIES
        XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
        XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
        XCODE_ATTRIBUTE_SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_LIST_DIR}/platforms/ios/WireGuard-Bridging-Header.h"
        XCODE_ATTRIBUTE_SWIFT_PRECOMPILE_BRIDGING_HEADER "NO"
        XCODE_ATTRIBUTE_SWIFT_OPTIMIZATION_LEVEL "-Onone"
        XCODE_ATTRIBUTE_SWIFT_OBJC_INTERFACE_HEADER_NAME "AmneziaVPN-Swift.h"
    )

    set_target_properties(${PROJECT} PROPERTIES
        OUTPUT_NAME "AmneziaVPN"
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_LIST_DIR}/ios/app/Info.plist.in
        MACOSX_BUNDLE_BUNDLE_NAME "AmneziaVPN"
        MACOSX_BUNDLE_BUNDLE_VERSION "${BUILD_ID}"
        MACOSX_BUNDLE_COPYRIGHT "MPL-2.0"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}"
        MACOSX_BUNDLE_INFO_STRING "AmneziaVPN"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${CMAKE_PROJECT_VERSION}-${BUILD_ID}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION}"
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}"
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_LIST_DIR}/ios/app/main.entitlements"
        XCODE_ATTRIBUTE_MARKETING_VERSION "${CMAKE_PROJECT_VERSION}"
        XCODE_GENERATE_SCHEME TRUE
        MACOSX_BUNDLE_ICON_FILE "AppIcon"

    )

    target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

    target_compile_options(${PROJECT} PRIVATE
        -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\"
        -DVPN_NE_BUNDLEID=\"${BUILD_IOS_APP_IDENTIFIER}.network-extension\"
    )


   target_sources(${PROJECT} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/Shared/Keychain.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/IPAddressRange.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/InterfaceConfiguration.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/Shared/Model/NETunnelProviderProtocol+Extension.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/TunnelConfiguration.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/Shared/Model/TunnelConfiguration+WgQuickConfig.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/Endpoint.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/Shared/Model/String+ArrayConversion.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/PeerConfiguration.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/DNSServer.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/WireGuardApp/LocalizationHelper.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/Shared/FileManager+Extension.swift
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/WireGuardKitC/x25519.c
        ${CMAKE_CURRENT_LIST_DIR}/3rd/wireguard-apple/Sources/WireGuardKit/PrivateKey.swift
    )

    target_sources(${PROJECT} PRIVATE
        platforms/ios/iosvpnprotocol.swift
        platforms/ios/ioslogger.swift
    )

    target_sources(${PROJECT} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/ios/app/launch.png
        ${CMAKE_CURRENT_LIST_DIR}/ios/app/AmneziaVPNLaunchScreen.storyboard
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_LIST_DIR}/ios/app/launch.png
        ${CMAKE_CURRENT_LIST_DIR}/ios/app/AmneziaVPNLaunchScreen.storyboard
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
    )

    target_sources(${PROJECT} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/ios/Media.xcassets
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_LIST_DIR}/ios/Media.xcassets
        PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"
    )

    add_subdirectory(ios/networkextension)
    add_dependencies(${PROJECT} networkextension)
    set_target_properties(${PROJECT} PROPERTIES XCODE_EMBED_APP_EXTENSIONS networkextension)

    set_property(TARGET ${PROJECT} PROPERTY XCODE_EMBED_FRAMEWORKS
        "${CMAKE_CURRENT_LIST_DIR}/3rd/OpenVPNAdapter/build/Release-iphoneos/OpenVPNAdapter.framework"
    )
    set_target_properties(${PROJECT} PROPERTIES XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON)
    set_target_properties(${PROJECT} PROPERTIES XCODE_LINK_BUILD_PHASE_MODE KNOWN_LOCATION)


    set_target_properties("networkextension"
        PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.amnezia.${PROJECT}.network-extension"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "X7UJ388FXK"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
        )

    set_target_properties("networkextension"
        PROPERTIES XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "org.amnezia.${PROJECT}.network-extension"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "X7UJ388FXK"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] "Apple Development"
        )

    set_target_properties (${PROJECT} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual)
    set_target_properties(${PROJECT} PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "match AppStore org.amnezia.AmneziaVPN")
    set_target_properties(${PROJECT} PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[variant=Debug] "match Development org.amnezia.AmneziaVPN")

    set_target_properties("networkextension" PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual)
    set_target_properties("networkextension" PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "match AppStore org.amnezia.AmneziaVPN.network-extension")
    set_target_properties("networkextension" PROPERTIES XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[variant=Debug] "match Development org.amnezia.AmneziaVPN.network-extension")

endif()

if(ANDROID)
    add_custom_command(
        TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_LIST_DIR}/android/AndroidManifest.xml 
            ${CMAKE_CURRENT_LIST_DIR}/android/build.gradle 
            ${CMAKE_CURRENT_LIST_DIR}/android/gradle/wrapper/gradle-wrapper.jar 
            ${CMAKE_CURRENT_LIST_DIR}/android/gradle/wrapper/gradle-wrapper.properties 
            ${CMAKE_CURRENT_LIST_DIR}/android/gradlew 
            ${CMAKE_CURRENT_LIST_DIR}/android/gradlew.bat 
            ${CMAKE_CURRENT_LIST_DIR}/android/gradle.properties 
            ${CMAKE_CURRENT_LIST_DIR}/android/res/values/libs.xml 
            ${CMAKE_CURRENT_LIST_DIR}/android/res/xml/fileprovider.xml
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/AuthHelper.java
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/IPCContract.kt
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/NotificationUtil.kt
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/OpenVPNThreadv3.kt
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/Prefs.kt
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/VPNLogger.kt
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/VPNService.kt
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/VPNServiceBinder.kt
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/qt/AmneziaApp.kt
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/qt/PackageManagerHelper.java
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/qt/VPNActivity.kt
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/qt/VPNApplication.java
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/qt/VPNClientBinder.kt
            ${CMAKE_CURRENT_LIST_DIR}/android/src/org/amnezia/vpn/qt/VPNPermissionHelper.kt
            ${CMAKE_CURRENT_BINARY_DIR}
    )

    set_property(TARGET ${PROJECT} PROPERTY
        QT_ANDROID_PACKAGE_SOURCE_DIR 
        ${CMAKE_CURRENT_LIST_DIR}/android
    )

    foreach(abi IN ITEMS ${QT_ANDROID_ABIS})
        if(CMAKE_ANDROID_ARCH_ABI STREQUAL ${abi})
            set(LIBS ${LIBS}
                ${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/lib/android/${abi}/libcrypto.a
                ${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/lib/android/${abi}/libssl.a
            )
        endif()

        set_property(TARGET ${PROJECT} PROPERTY QT_ANDROID_EXTRA_LIBS
            ${CMAKE_CURRENT_LIST_DIR}/android/lib/wireguard/${abi}/libwg.so
            ${CMAKE_CURRENT_LIST_DIR}/android/lib/wireguard/${abi}/libwg-go.so
            ${CMAKE_CURRENT_LIST_DIR}/android/lib/wireguard/${abi}/libwg-quick.so

            ${CMAKE_CURRENT_LIST_DIR}/android/lib/openvpn/${abi}/libjbcrypto.so
            ${CMAKE_CURRENT_LIST_DIR}/android/lib/openvpn/${abi}/libopenvpn.so
            ${CMAKE_CURRENT_LIST_DIR}/android/lib/openvpn/${abi}/libopvpnutil.so
            ${CMAKE_CURRENT_LIST_DIR}/android/lib/openvpn/${abi}/libovpn3.so
            ${CMAKE_CURRENT_LIST_DIR}/android/lib/openvpn/${abi}/libovpnexec.so
        )
    endforeach()

endif()

target_link_libraries(${PROJECT} PRIVATE ${LIBS})

if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    qt_add_repc_replicas(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc_interface.rep)
    qt_add_repc_replicas(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc_process_interface.rep)
endif()

# deploy artifacts required to run the application to the debug build folder
if(WIN32)
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        set(DEPLOY_ARTIFACT_PATH "windows/x64")
    else()
        set(DEPLOY_ARTIFACT_PATH "windows/x32")
    endif()
elseif(LINUX)
    set(DEPLOY_ARTIFACT_PATH "linux/client")
elseif(APPLE AND NOT IOS)
    set(DEPLOY_ARTIFACT_PATH "macos")
endif()

if(NOT IOS)
    add_custom_command(
        TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,copy_directory,true>
        ${CMAKE_SOURCE_DIR}/deploy/data/${DEPLOY_ARTIFACT_PATH}
        $<TARGET_FILE_DIR:${PROJECT}>
        COMMAND_EXPAND_LISTS
    )
endif()

if(WIN32)
    add_custom_command(
        TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,copy,true>
        $<TARGET_FILE_DIR:${PROJECT}>/../service/wireguard-service/wireguard-service.exe
        $<TARGET_FILE_DIR:${PROJECT}>/wireguard/wireguard-service.exe
        COMMAND_EXPAND_LISTS
    )
endif()
if(IOS)
 #include(cmake/ios-arch-fixup.cmake)
endif()
