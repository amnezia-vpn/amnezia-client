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
    ${FW_USERNOTIFICATIONS}
    ${FW_NETWORK_EXTENSION}
)

set_target_properties(${PROJECT} PROPERTIES MACOSX_BUNDLE TRUE)
set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)


set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/macos_util.h
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ikev2_vpn_protocol_mac.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/macos_util.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols/ikev2_vpn_protocol_mac.mm
)

set(ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/images/app.icns)
set(MACOSX_BUNDLE_ICON_FILE app.icns)
set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
set(SOURCES ${SOURCES} ${ICON_FILE})

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


