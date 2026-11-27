enable_language(Swift)
message("Client message >> macos build >> ${CLIENT_MACOS_NE_TARGET_NAME}")
set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

add_executable(${CLIENT_MACOS_NE_TARGET_NAME})

message("executable_path is: @executable_path/../../Frameworks")

# The plist is consumed by Xcode directly, so branding variables have to be
# substituted by CMake beforehand
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in ${CMAKE_CURRENT_BINARY_DIR}/Info.plist)

set_target_properties(${CLIENT_MACOS_NE_TARGET_NAME} PROPERTIES
    XCODE_PRODUCT_TYPE com.apple.product-type.app-extension
    BUNDLE_EXTENSION appex
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${BUILD_IOS_APP_IDENTIFIER}.network-extension"
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${CLIENT_MACOS_NE_ENTITLEMENTS_PATH}
    XCODE_ATTRIBUTE_MARKETING_VERSION "${APP_MAJOR_VERSION}"
    XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    XCODE_ATTRIBUTE_PRODUCT_NAME "${CLIENT_NETWORK_EXTENSION_NAME}"

    XCODE_ATTRIBUTE_APPLICATION_EXTENSION_API_ONLY "YES"

    XCODE_ATTRIBUTE_INFOPLIST_FILE ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
    XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../../../../Frameworks @loader_path/../../../../Frameworks"

    XCODE_LINK_BUILD_PHASE_MODE KNOWN_LOCATION
)

if(DEPLOY)
    message("DEPLOY is ON")
    set_target_properties(${CLIENT_MACOS_NE_TARGET_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Distribution"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY[variant=Debug] "Apple Development"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Manual
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${CLIENT_MACOS_NE_PROVISIONING_PROFILE_SPECIFIER}"
        XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER[variant=Debug] "${CLIENT_MACOS_NE_PROVISIONING_PROFILE_SPECIFIER_DEBUG}"
    )
else()
    set_target_properties(${CLIENT_MACOS_NE_TARGET_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
    )
endif()

set_target_properties(${CLIENT_MACOS_NE_TARGET_NAME} PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"
    XCODE_ATTRIBUTE_SWIFT_OPTIMIZATION_LEVEL "-Onone"
)

set_target_properties("${CLIENT_MACOS_NE_TARGET_NAME}" PROPERTIES
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${BUILD_VPN_DEVELOPMENT_TEAM}"
)

find_library(FW_ASSETS_LIBRARY AssetsLibrary)
find_library(FW_MOBILE_CORE MobileCoreServices)
find_library(FW_UI_KIT UIKit)
find_library(FW_LIBRESOLV libresolv.9.tbd)


# Set the root directory
set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/../..)

target_link_libraries(${CLIENT_MACOS_NE_TARGET_NAME} PRIVATE ${FW_LIBRESOLV})

target_compile_options(${CLIENT_MACOS_NE_TARGET_NAME} PRIVATE -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\")
target_compile_options(${CLIENT_MACOS_NE_TARGET_NAME} PRIVATE -DNETWORK_EXTENSION=1)

target_sources(${CLIENT_MACOS_NE_TARGET_NAME} PRIVATE
    ${CLIENT_ROOT_DIR}/platforms/ios/NELogController.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/Log.swift
    ${CLIENT_GROUP_IDENTIFIER_SWIFT_FILE}
    ${CLIENT_ROOT_DIR}/platforms/ios/LogRecord.swift
    ${CLIENT_ROOT_DIR}/platforms/ios/PacketTunnelProvider.swift
)

target_sources(${CLIENT_MACOS_NE_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/PrivacyInfo.xcprivacy
)

set_property(TARGET ${CLIENT_MACOS_NE_TARGET_NAME} APPEND PROPERTY RESOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/PrivacyInfo.xcprivacy
)

target_include_directories(${CLIENT_MACOS_NE_TARGET_NAME} PRIVATE ${CLIENT_ROOT_DIR})
target_include_directories(${CLIENT_MACOS_NE_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

include(${CLIENT_ROOT_DIR}/cmake/xtunApple.cmake)
target_link_libraries(${CLIENT_MACOS_NE_TARGET_NAME} PRIVATE ${XTUN_APPLE_XCFRAMEWORK})
