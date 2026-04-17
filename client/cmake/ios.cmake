message("Client iOS build")
set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)
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
find_library(FW_NETWORKEXTENSION NetworkExtension)
find_library(FW_NETWORK Network)
find_library(FW_QUARTZCORE QuartzCore)
find_library(FW_OPENGLES OpenGLES)
find_library(FW_COREVIDEO CoreVideo)

set(LIBS ${LIBS}
    ${FW_AUTHENTICATIONSERVICES}
    ${FW_UIKIT}
    ${FW_AVFOUNDATION}
    ${FW_FOUNDATION}
    ${FW_STOREKIT}
    ${FW_USERNOTIFICATIONS}
    ${FW_NETWORKEXTENSION}
    ${FW_NETWORK}
    ${FW_QUARTZCORE}
    ${FW_OPENGLES}
    ${FW_COREVIDEO}
)


set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/ios_controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/ios_controller_wrapper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/iosnotificationhandler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/QtAppDelegate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/StoreKitController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/QtAppDelegate-C-Interface.h
)
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/ios_controller.h PROPERTIES OBJECTIVE_CPP_HEADER TRUE)


set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/ios_controller.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/ios_controller_wrapper.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/iosnotificationhandler.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/iosglue.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/QRCodeReaderBase.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/QtAppDelegate.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/StoreKitController.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/FBLinkSceneDelegateHooks.mm
)


target_include_directories(${PROJECT} PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})


set_target_properties(${PROJECT} PROPERTIES
    LINKER_LANGUAGE Swift
    XCODE_LINK_BUILD_PHASE_MODE KNOWN_LOCATION
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/Info.plist.in
    MACOSX_BUNDLE_ICON_FILE "AppIcon"
    MACOSX_BUNDLE_INFO_STRING "FBLink"
    MACOSX_BUNDLE_BUNDLE_NAME "FBLink"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${APPLE_PROJECT_VERSION}-${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${APPLE_PROJECT_VERSION}"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/ios/app/main.entitlements"
    XCODE_ATTRIBUTE_MARKETING_VERSION "${APPLE_PROJECT_VERSION}"
    XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    XCODE_ATTRIBUTE_PRODUCT_NAME "FBLink"
    XCODE_ATTRIBUTE_BUNDLE_INFO_STRING "FBLink"
    XCODE_GENERATE_SCHEME TRUE
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphonesimulator iphoneos"
    XCODE_ATTRIBUTE_SUPPORTS_MACCATALYST "NO"
    XCODE_ATTRIBUTE_SUPPORTS_MAC_DESIGNED_FOR_IPHONE_IPAD "NO"
    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON
    XCODE_LINK_BUILD_PHASE_MODE KNOWN_LOCATION
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
    XCODE_ATTRIBUTE_LIBRARY_SEARCH_PATHS "$(inherited) $(TOOLCHAIN_DIR)/usr/lib/swift/$(PLATFORM_NAME) $(SDKROOT)/usr/lib/swift"
    XCODE_ATTRIBUTE_ALWAYS_EMBED_SWIFT_STANDARD_LIBRARIES "YES"
    XCODE_ATTRIBUTE_EMBEDDED_CONTENT_CONTAINS_SWIFT "YES"
    XCODE_ATTRIBUTE_OTHER_LDFLAGS "$(inherited) -ObjC"
    XCODE_EMBED_APP_EXTENSIONS networkextension
)

if(DEPLOY)
    set_target_properties(${PROJECT} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] "Apple Development"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "distr ios.org.fblink.FBLink"
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[variant=Debug] "dev ios.org.fblink.FBLink"
    )
else()
    set_target_properties(${PROJECT} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED[variant=Debug] "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED[variant=Debug] "NO"
    )
endif()

set_target_properties(${PROJECT} PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
    XCODE_ATTRIBUTE_SWIFT_PRECOMPILE_BRIDGING_HEADER "NO"
    XCODE_ATTRIBUTE_SWIFT_OBJC_INTERFACE_HEADER_NAME "FBLink-Swift.h"
)
if(DEFINED BUILD_IOS_DEVELOPMENT_TEAM AND NOT BUILD_IOS_DEVELOPMENT_TEAM STREQUAL "")
    set_target_properties(${PROJECT} PROPERTIES
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${BUILD_IOS_DEVELOPMENT_TEAM}"
    )
endif()
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(${PROJECT} PRIVATE
    -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\"
    -DVPN_NE_BUNDLEID=\"${BUILD_IOS_APP_IDENTIFIER}.network-extension\"
)

set(WG_APPLE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd/amneziawg-apple/Sources)

target_sources(${PROJECT} PRIVATE
#    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/iosvpnprotocol.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKitC/x25519.c
    ${CLIENT_ROOT_DIR}/platforms/ios/fblink_ios_bridge.mm
    ${CLIENT_ROOT_DIR}/platforms/ios/LogController.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/Log.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/LogRecord.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/ScreenProtection.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/VPNCController.swift
)

target_sources(${PROJECT} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/FBLinkLaunchScreen.storyboard
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/Media.xcassets
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/PrivacyInfo.xcprivacy
)

set_property(TARGET ${PROJECT} APPEND PROPERTY RESOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/FBLinkLaunchScreen.storyboard
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/Media.xcassets
    ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/PrivacyInfo.xcprivacy
)

add_subdirectory(ios/networkextension)
add_dependencies(${PROJECT} networkextension)

set_property(TARGET ${PROJECT} PROPERTY XCODE_EMBED_FRAMEWORKS
    "${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/apple/OpenVPNAdapter-ios/OpenVPNAdapter.framework"
)

set(CMAKE_XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/apple/OpenVPNAdapter-ios/)
target_link_libraries("networkextension" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/apple/OpenVPNAdapter-ios/OpenVPNAdapter.framework")
