message("Client iOS build")
set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)
set(APPLE_PROJECT_VERSION ${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH})
set(AMNEZIA_IOS_APPLETV ${AMNEZIA_IOS_ENABLE_APPLETV_TARGET})

if(AMNEZIA_IOS_APPLETV)
    message("Apple TV target mode is ON")
    set(CMAKE_OSX_DEPLOYMENT_TARGET 17.0)
    set(QT_NO_SET_DEFAULT_IOS_LAUNCH_SCREEN TRUE)
    set(QT_NO_ADD_IOS_LAUNCH_SCREEN_TO_BUNDLE TRUE)
    set(IOS_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/Info-tvOS.plist.in)
    set(IOS_LAUNCHSCREEN_STORYBOARD ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/tvOS/AmneziaVPNLaunchScreen.storyboard)
else()
    message("Apple TV target mode is OFF")
    set(IOS_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/Info.plist.in)
    set(IOS_LAUNCHSCREEN_STORYBOARD ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/AmneziaVPNLaunchScreen.storyboard)
endif()


enable_language(OBJC)
enable_language(OBJCXX)
enable_language(Swift)

find_package(Qt6 REQUIRED COMPONENTS ShaderTools)
set(LIBS ${LIBS} Qt6::ShaderTools)

if(AMNEZIA_IOS_APPLETV)
    # Use framework linker flags directly for tvOS to avoid iPhoneOS SDK absolute paths.
    set(FW_AUTHENTICATIONSERVICES "-framework AuthenticationServices")
    set(FW_UIKIT "-framework UIKit")
    set(FW_AVFOUNDATION "-framework AVFoundation")
    set(FW_FOUNDATION "-framework Foundation")
    set(FW_STOREKIT "-framework StoreKit")
    set(FW_USERNOTIFICATIONS "-framework UserNotifications")
else()
    find_library(FW_AUTHENTICATIONSERVICES AuthenticationServices)
    find_library(FW_UIKIT UIKit)
    find_library(FW_AVFOUNDATION AVFoundation)
    find_library(FW_FOUNDATION Foundation)
    find_library(FW_STOREKIT StoreKit)
    find_library(FW_USERNOTIFICATIONS UserNotifications)
    find_library(FW_NETWORKEXTENSION NetworkExtension)
endif()

set(LIBS ${LIBS}
    ${FW_AUTHENTICATIONSERVICES}
    ${FW_UIKIT}
    ${FW_AVFOUNDATION}
    ${FW_FOUNDATION}
    ${FW_STOREKIT}
    ${FW_USERNOTIFICATIONS}
)

if(NOT AMNEZIA_IOS_APPLETV)
    set(LIBS ${LIBS} ${FW_NETWORKEXTENSION})
endif()


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
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/AmneziaSceneDelegateHooks.mm
)


target_include_directories(${PROJECT} PRIVATE ${Qt6Gui_PRIVATE_INCLUDE_DIRS})


set_target_properties(${PROJECT} PROPERTIES
    XCODE_LINK_BUILD_PHASE_MODE KNOWN_LOCATION
    MACOSX_BUNDLE_INFO_PLIST ${IOS_INFO_PLIST}
    MACOSX_BUNDLE_ICON_FILE "AppIcon"
    MACOSX_BUNDLE_INFO_STRING "AmneziaVPN"
    MACOSX_BUNDLE_BUNDLE_NAME "AmneziaVPN"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${APPLE_PROJECT_VERSION}-${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${APPLE_PROJECT_VERSION}"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}"
    XCODE_ATTRIBUTE_MARKETING_VERSION "${APPLE_PROJECT_VERSION}"
    XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    XCODE_ATTRIBUTE_PRODUCT_NAME "AmneziaVPN"
    XCODE_ATTRIBUTE_BUNDLE_INFO_STRING "AmneziaVPN"
    XCODE_GENERATE_SCHEME TRUE
    XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
    XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon"
    XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY ON
    XCODE_LINK_BUILD_PHASE_MODE KNOWN_LOCATION
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/Frameworks"
    XCODE_EMBED_APP_EXTENSIONS networkextension
)

if(AMNEZIA_IOS_APPLETV)
    set_target_properties(${PROJECT} PROPERTIES
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
    )
    set_target_properties(${PROJECT} PROPERTIES
        QT_IOS_PERMISSIONS ""
    )
else()
    set_target_properties(${PROJECT} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/ios/app/main.entitlements"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    )
endif()

if(DEFINED DEPLOY)
    set_target_properties(${PROJECT} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] "Apple Development"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "distr ios.org.amnezia.AmneziaVPN"
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[variant=Debug] "dev ios.org.amnezia.AmneziaVPN"
    )
else()
    set_target_properties(${PROJECT} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
    )
endif()

set_target_properties(${PROJECT} PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
    XCODE_ATTRIBUTE_SWIFT_PRECOMPILE_BRIDGING_HEADER "NO"
    XCODE_ATTRIBUTE_SWIFT_OBJC_INTERFACE_HEADER_NAME "AmneziaVPN-Swift.h"
    XCODE_ATTRIBUTE_SWIFT_OBJC_INTEROP_MODE "objcxx"
)
set_target_properties(${PROJECT} PROPERTIES
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "X7UJ388FXK"
)
target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_compile_options(${PROJECT} PRIVATE
    -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\"
    -DVPN_NE_BUNDLEID=\"${BUILD_IOS_APP_IDENTIFIER}.network-extension\"
)

if(AMNEZIA_IOS_APPLETV)
    # qscnetworkreachability plugin links IOKit, which is unavailable on tvOS.
    qt_import_plugins(${PROJECT}
        NO_DEFAULT
        INCLUDE
            QIOSIntegrationPlugin
            QJpegPlugin
            QSvgPlugin
            QGifPlugin
            QICOPlugin
            QSvgIconPlugin
            QSecureTransportBackendPlugin
        EXCLUDE
            QSCNetworkReachabilityNetworkInformationPlugin
            QDarwinCameraPermissionPlugin
    )

    # Static tvOS Qt build doesn't auto-link these plugin archives into the
    # Xcode target, but the app entry point lives in QIOSIntegrationPlugin.
    set(_amnezia_tvos_static_plugins
        Qt6::QIOSIntegrationPlugin
        Qt6::QIOSIntegrationPlugin_init
        Qt6::QJpegPlugin
        Qt6::QJpegPlugin_init
        Qt6::QSvgPlugin
        Qt6::QSvgPlugin_init
        Qt6::QGifPlugin
        Qt6::QGifPlugin_init
        Qt6::QICOPlugin
        Qt6::QICOPlugin_init
        Qt6::QSvgIconPlugin
        Qt6::QSvgIconPlugin_init
        Qt6::QSecureTransportBackendPlugin
        Qt6::QSecureTransportBackendPlugin_init
    )
    foreach(_amnezia_tvos_static_plugin IN LISTS _amnezia_tvos_static_plugins)
        if(TARGET ${_amnezia_tvos_static_plugin})
            target_link_libraries(${PROJECT} PRIVATE ${_amnezia_tvos_static_plugin})
        endif()
    endforeach()
    unset(_amnezia_tvos_static_plugin)
    unset(_amnezia_tvos_static_plugins)

    # Qt 6.9.2 iOS package links IOKit via Qt6::Core interface, but tvOS SDK
    # does not provide IOKit. Strip this single framework for Apple TV builds.
    get_target_property(_qtcore_iface_libs Qt6::Core INTERFACE_LINK_LIBRARIES)
    if(_qtcore_iface_libs)
        string(REPLACE "-framework IOKit;" "" _qtcore_iface_libs "${_qtcore_iface_libs}")
        string(REPLACE ";-framework IOKit" "" _qtcore_iface_libs "${_qtcore_iface_libs}")
        set_property(TARGET Qt6::Core PROPERTY INTERFACE_LINK_LIBRARIES "${_qtcore_iface_libs}")
    endif()
endif()

set(WG_APPLE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rd/amneziawg-apple/Sources)

target_sources(${PROJECT} PRIVATE
#    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/ios/iosvpnprotocol.swift
    ${WG_APPLE_SOURCE_DIR}/WireGuardKitC/x25519.c
    ${CLIENT_ROOT_DIR}/platforms/ios/LogController.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/Log.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/LogRecord.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/ScreenProtection.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/VPNCController.swift
)

if(IOS_LAUNCHSCREEN_STORYBOARD)
    target_sources(${PROJECT} PRIVATE
        ${IOS_LAUNCHSCREEN_STORYBOARD}
        ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/Media.xcassets
        ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/PrivacyInfo.xcprivacy
    )

    set_property(TARGET ${PROJECT} APPEND PROPERTY RESOURCE
        ${IOS_LAUNCHSCREEN_STORYBOARD}
        ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/Media.xcassets
        ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/PrivacyInfo.xcprivacy
    )
else()
    target_sources(${PROJECT} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/Media.xcassets
        ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/PrivacyInfo.xcprivacy
    )

    set_property(TARGET ${PROJECT} APPEND PROPERTY RESOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/Media.xcassets
        ${CMAKE_CURRENT_SOURCE_DIR}/ios/app/PrivacyInfo.xcprivacy
    )
endif()

add_subdirectory(ios/networkextension)
add_dependencies(${PROJECT} networkextension)

if(NOT AMNEZIA_IOS_APPLETV)
    set_property(TARGET ${PROJECT} PROPERTY XCODE_EMBED_FRAMEWORKS
        "${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/apple/OpenVPNAdapter-ios/OpenVPNAdapter.framework"
    )

    set(CMAKE_XCODE_ATTRIBUTE_FRAMEWORK_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/apple/OpenVPNAdapter-ios/)
    target_link_libraries("networkextension" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rd-prebuilt/3rd-prebuilt/openvpn/apple/OpenVPNAdapter-ios/OpenVPNAdapter.framework")
endif()
