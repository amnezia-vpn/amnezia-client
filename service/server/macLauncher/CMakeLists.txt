set(PROJECT macLauncher)

set(CMAKE_Swift_LANGUAGE_VERSION "5.0")

project(${PROJECT} LANGUAGES Swift)

add_executable(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/main.swift)

target_link_libraries(${PROJECT} PRIVATE serverMacShared SecureXPC)
include(${CMAKE_SOURCE_DIR}/cmake/macos-signing.cmake)

set_target_properties(${PROJECT} PROPERTIES
    XCODE_ATTRIBUTE_SWIFT_VERSION "5.0"
    MACOSX_BUNDLE_BUNDLE_VERSION "${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_LONG_VERSION_STRING "${APPLE_PROJECT_VERSION}-${CMAKE_PROJECT_VERSION_TWEAK}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${APPLE_PROJECT_VERSION}"
)

configure_file(${CMAKE_CURRENT_LIST_DIR}/Info.plist.in ${CMAKE_BINARY_DIR}/service/macLauncher/Info.plist)
configure_file(${CMAKE_CURRENT_LIST_DIR}/Launchd.plist.in ${CMAKE_BINARY_DIR}/service/macLauncher/Launchd.plist)

target_link_options(${PROJECT} PRIVATE LINKER:-sectcreate,__TEXT,__info_plist,${CMAKE_BINARY_DIR}/service/macLauncher/Info.plist)
target_link_options(${PROJECT} PRIVATE LINKER:-sectcreate,__TEXT,__launchd_plist,${CMAKE_BINARY_DIR}/service/macLauncher/Launchd.plist)
