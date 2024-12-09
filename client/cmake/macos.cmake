message("MAC build")

enable_language(Swift)

set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "" FORCE)

include(${CMAKE_SOURCE_DIR}/cmake/InitializeSwift.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/GenerateSwiftHeader.cmake)

find_library(FW_SYSTEMCONFIG SystemConfiguration)
find_library(FW_SERVICEMGMT ServiceManagement)
find_library(FW_SECURITY Security)
find_library(FW_COREWLAN CoreWLAN)
find_library(FW_NETWORK Network)
find_library(FW_USER_NOTIFICATIONS UserNotifications)
find_library(FW_NETWORK_EXTENSION NetworkExtension)

set(LIBS ${LIBS}
        ${FW_SYSTEMCONFIG}
        ${FW_SERVICEMGMT}
        ${FW_SECURITY}
        ${FW_COREWLAN}
        ${FW_NETWORK}
        ${FW_USERNOTIFICATIONS}
        ${FW_NETWORK_EXTENSION}
)

set_target_properties(${PROJECT} PROPERTIES MACOSX_BUNDLE TRUE)

set(SPARKLE_ED_PUBLIC_KEY $ENV{SPARKLE_ED_PUBLIC_KEY})
set(SPARKLE_FEED_URL $ENV{SPARKLE_FEED_URL})

set_target_properties(${PROJECT} PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES "YES"

        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/macos/app/Info.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER "${BUILD_OSX_APP_IDENTIFIER}"
        MACOSX_BUNDLE_INFO_STRING "ZloVPN"
        MACOSX_BUNDLE_BUNDLE_NAME "ZloVPN"
        MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${APPLE_PROJECT_VERSION}-${CMAKE_PROJECT_VERSION_TWEAK}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${APPLE_PROJECT_VERSION}"

        XCODE_ATTRIBUTE_PRODUCT_NAME "ZloVPN"
        XCODE_ATTRIBUTE_BUNDLE_INFO_STRING "ZloVPN"
        XCODE_ATTRIBUTE_CURRENT_PROJECT_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
        XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
)

include(${CMAKE_SOURCE_DIR}/cmake/macos-signing.cmake)
target_sources(${PROJECT} PRIVATE ${CMAKE_SOURCE_DIR}/cmake/ZloVPN.entitlements)

set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/macos_util.h
)

set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/ui/macos_util.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/macos/FirstSetupController.swift
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/macos/AutoUpdater.swift
        ${CMAKE_CURRENT_SOURCE_DIR}/platforms/macos/AuthorizationHelper.swift
)

target_link_libraries(${PROJECT} PRIVATE SecureXPC serverMacShared)

set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/images/app.icns)
set(MACOSX_BUNDLE_ICON_FILE app.icns)
set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
set(SOURCES ${SOURCES} ${ICON_FILE})

target_compile_options(${PROJECT} PRIVATE
        -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\"
        -DVPN_NE_BUNDLEID=\"${BUILD_IOS_APP_IDENTIFIER}.network-extension\"
)

# todo: support multiple binaries
set(SWIFT_OPTIONS "-cxx-interoperability-mode=default" -target ${CMAKE_OSX_ARCHITECTURES}-apple-macosx${CMAKE_OSX_DEPLOYMENT_TARGET})
target_compile_options(${PROJECT} PUBLIC $<$<COMPILE_LANGUAGE:Swift>:${SWIFT_OPTIONS}>)

_swift_generate_cxx_header(${PROJECT}
        ${CMAKE_CURRENT_BINARY_DIR}/AmneziaVPN-Swift.h)

# Get SDK path
execute_process(
        COMMAND sh -c " xcrun --sdk macosx --show-sdk-path"
        OUTPUT_VARIABLE OSX_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("OSX_SDK_PATH is: ${OSX_SDK_PATH}")


