message("MAC build")

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
    ${FW_USER_NOTIFICATIONS}
    ${FW_NETWORK_EXTENSION}
)

set_target_properties(${PROJECT} PROPERTIES 
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
)

set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/utils/macosUtil.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/utils/macosUtil.mm
)



check_apple_app_icon(${CLIENT_MACOS_APP_ICON_PATH})

set(APP_ICON_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/appicon)
set(APP_ICON_FILES ${APP_ICON_OUTPUT_DIR}/Assets.car ${APP_ICON_OUTPUT_DIR}/AppIcon.icns)
file(GLOB_RECURSE APP_ICON_INPUTS CONFIGURE_DEPENDS "${CLIENT_MACOS_APP_ICON_PATH}/*")

add_custom_command(
    OUTPUT ${APP_ICON_FILES}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_ICON_OUTPUT_DIR}
    COMMAND xcrun actool --output-format human-readable-text --warnings --errors
            --platform macosx
            --minimum-deployment-target ${CMAKE_OSX_DEPLOYMENT_TARGET}
            --app-icon AppIcon
            --output-partial-info-plist ${APP_ICON_OUTPUT_DIR}/partial.plist
            --compile ${APP_ICON_OUTPUT_DIR}
            ${CLIENT_MACOS_APP_ICON_PATH}
    DEPENDS ${APP_ICON_INPUTS}
    COMMENT "Compiling macOS app icon"
    VERBATIM
)

set(MACOSX_BUNDLE_ICON_FILE AppIcon)
set_source_files_properties(${APP_ICON_FILES} PROPERTIES GENERATED TRUE MACOSX_PACKAGE_LOCATION Resources)
set(SOURCES ${SOURCES} ${APP_ICON_FILES})

add_custom_command(TARGET ${PROJECT} POST_BUILD
    COMMAND plutil -replace CFBundleIconName -string AppIcon $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT}>/Info.plist
    VERBATIM
)

target_compile_options(${PROJECT} PRIVATE
    -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\"
    -DVPN_NE_BUNDLEID=\"${BUILD_IOS_APP_IDENTIFIER}.network-extension\"
)

# Get SDK path
execute_process(
    COMMAND sh -c "xcrun --sdk macosx --show-sdk-path"
    OUTPUT_VARIABLE OSX_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("OSX_SDK_PATH is: ${OSX_SDK_PATH}")

