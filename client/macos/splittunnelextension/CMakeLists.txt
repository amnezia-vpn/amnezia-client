cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT "AmneziaVPNSplitTunnel")

project(${PROJECT})

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in
    ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
    @ONLY
)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/AmneziaVPNSplitTunnel.entitlements.in
    ${CMAKE_CURRENT_BINARY_DIR}/AmneziaVPNSplitTunnel.entitlements
    @ONLY
)
set(CLIENT_MACOS_ST_ENTITLEMENTS_PATH
    ${CMAKE_CURRENT_BINARY_DIR}/AmneziaVPNSplitTunnel.entitlements
    CACHE FILEPATH "Entitlements for the macOS split-tunnel system extension" FORCE)

add_executable(${PROJECT} MACOSX_BUNDLE
    main.mm
    TransparentProxyProvider.mm
    FlowTCP.mm
    FlowUDP.mm
    AppPolicy.mm
    Settings.mm
    Utils.mm
)

set_target_properties(${PROJECT} PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION systemextension
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_BINARY_DIR}/Info.plist
    MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT}"
    MACOSX_BUNDLE_GUI_IDENTIFIER "${CLIENT_MACOS_ST_BUNDLE_ID}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_MAJOR_VERSION}"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CLIENT_MACOS_ST_BUNDLE_VERSION}"
    MACOSX_BUNDLE_COPYRIGHT "AmneziaVPN"
    OUTPUT_NAME "${CLIENT_MACOS_ST_BUNDLE_ID}"
)

set_source_files_properties(
    main.mm
    TransparentProxyProvider.mm
    FlowTCP.mm
    FlowUDP.mm
    AppPolicy.mm
    Settings.mm
    Utils.mm
    PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
)

find_library(ST_FW_FOUNDATION Foundation REQUIRED)
find_library(ST_FW_NETWORK_EXTENSION NetworkExtension REQUIRED)
find_library(ST_FW_NETWORK Network REQUIRED)
find_library(ST_FW_SECURITY Security REQUIRED)

target_link_libraries(${PROJECT} PRIVATE
    ${ST_FW_FOUNDATION}
    ${ST_FW_NETWORK_EXTENSION}
    ${ST_FW_NETWORK}
    ${ST_FW_SECURITY}
)

target_include_directories(${PROJECT} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(${PROJECT} PRIVATE
    "CLIENT_MACOS_ST_BUNDLE_ID=\"${CLIENT_MACOS_ST_BUNDLE_ID}\""
)

install(DIRECTORY "$<TARGET_BUNDLE_DIR:${PROJECT}>/"
    DESTINATION "${CMAKE_INSTALL_BINDIR}/${CLIENT_APPLICATION_NAME}.app/Contents/Library/SystemExtensions/${CLIENT_MACOS_ST_BUNDLE_ID}.systemextension"
    COMPONENT AmneziaVPN
    USE_SOURCE_PERMISSIONS
)
