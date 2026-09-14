message("MAC build")

find_library(FW_SYSTEMCONFIG SystemConfiguration)
find_library(FW_SERVICEMGMT ServiceManagement)
find_library(FW_SECURITY Security)
find_library(FW_COREWLAN CoreWLAN)
find_library(FW_NETWORK Network)
find_library(FW_USER_NOTIFICATIONS UserNotifications)
find_library(FW_NETWORK_EXTENSION NetworkExtension)
find_library(FW_SYSTEM_EXTENSIONS SystemExtensions)
find_library(FW_APPKIT AppKit)

set(LIBS ${LIBS}
    ${FW_SYSTEMCONFIG}
    ${FW_SERVICEMGMT}
    ${FW_SECURITY}
    ${FW_COREWLAN}
    ${FW_NETWORK}
    ${FW_USER_NOTIFICATIONS}
    ${FW_NETWORK_EXTENSION}
    ${FW_SYSTEM_EXTENSIONS}
    ${FW_APPKIT}
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/macos/app/app-developerid.entitlements.in
    ${CMAKE_CURRENT_BINARY_DIR}/app-developerid.entitlements
    @ONLY
)
set(CLIENT_MACOS_APP_DEVELOPERID_ENTITLEMENTS_PATH
    ${CMAKE_CURRENT_BINARY_DIR}/app-developerid.entitlements
    CACHE FILEPATH "Developer ID entitlements for the .pkg app bundle" FORCE)

set(ICON_FILE ${CLIENT_MACOS_APP_ICNS_PATH})
get_filename_component(MACOSX_BUNDLE_ICON_FILE "${CLIENT_MACOS_APP_ICNS_PATH}" NAME)
set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
set(SOURCES ${SOURCES} ${ICON_FILE})

# CMake only substitutes MACOSX_BUNDLE_* into MACOSX_BUNDLE_INFO_PLIST, so
# ${MACOSX_DEPLOYMENT_TARGET} used to expand to an empty <string/>. Pre-configure
# the template with @ONLY (which leaves ${...} alone) and drop the key entirely
# when no deployment target is pinned.
if(CMAKE_OSX_DEPLOYMENT_TARGET)
    set(AMN_MACOS_MIN_VERSION_ENTRY
        "\t<key>LSMinimumSystemVersion</key>\n\t<string>${CMAKE_OSX_DEPLOYMENT_TARGET}</string>\n")
else()
    set(AMN_MACOS_MIN_VERSION_ENTRY "")
    message(STATUS "CMAKE_OSX_DEPLOYMENT_TARGET is not set: the app bundle will not declare LSMinimumSystemVersion")
endif()
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/macos/app/Info-developerid.plist.in
    ${CMAKE_CURRENT_BINARY_DIR}/Info-developerid.plist.in
    @ONLY
)

set_target_properties(${PROJECT} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info-developerid.plist.in
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${BUILD_OSX_APP_IDENTIFIER}"
    MACOSX_BUNDLE_BUNDLE_NAME "${CLIENT_APPLICATION_NAME}"
    MACOSX_BUNDLE_ICON_FILE "${MACOSX_BUNDLE_ICON_FILE}"
)

set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/utils/macosUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/macos/splittunnel/macosSplitTunnelManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/macos/splittunnel/macosAppInfo.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/utils/macosUtil.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/macos/splittunnel/macosSplitTunnelManager.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/macos/splittunnel/macosAppInfo.mm
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/macos/splittunnel/macosSplitTunnelManager.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/platforms/macos/splittunnel/macosAppInfo.mm
    PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
)

target_compile_options(${PROJECT} PRIVATE
    -DGROUP_ID=\"${BUILD_IOS_GROUP_IDENTIFIER}\"
    -DVPN_NE_BUNDLEID=\"${BUILD_IOS_APP_IDENTIFIER}.network-extension\"
    -DCLIENT_MACOS_ST_BUNDLE_ID=\"${CLIENT_MACOS_ST_BUNDLE_ID}\"
)

# Get SDK path
execute_process(
    COMMAND sh -c "xcrun --sdk macosx --show-sdk-path"
    OUTPUT_VARIABLE OSX_SDK_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("OSX_SDK_PATH is: ${OSX_SDK_PATH}")

add_subdirectory(macos/splittunnelextension)

add_dependencies(${PROJECT} ${CLIENT_MACOS_ST_TARGET_NAME})
add_custom_command(TARGET ${PROJECT} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT}>/Library/SystemExtensions"
    COMMAND ${CMAKE_COMMAND} -E rm -rf
        "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT}>/Library/SystemExtensions/$<TARGET_FILE_NAME:${CLIENT_MACOS_ST_TARGET_NAME}>.systemextension"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "$<TARGET_BUNDLE_DIR:${CLIENT_MACOS_ST_TARGET_NAME}>"
        "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT}>/Library/SystemExtensions/$<TARGET_FILE_NAME:${CLIENT_MACOS_ST_TARGET_NAME}>.systemextension"
    COMMENT "Embed split-tunnel system extension into app bundle"
)
