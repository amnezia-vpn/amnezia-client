cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT FBLink)
project(${PROJECT})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "Autogen")
set_property(GLOBAL PROPERTY AUTOMOC_TARGETS_FOLDER "Autogen")
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Autogen")

set(PACKAGES
    Core Gui Network Xml
    Quick Svg QuickControls2
    Concurrent LinguistTools
)

# Qt5 Compatibility module is not shipped with the default iOS Qt kit.
# Desktop, Android and macOS builds still require it.
if(NOT IOS)
    set(PACKAGES ${PACKAGES} Core5Compat)
endif()

execute_process(
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMAND git rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

add_definitions(-DPROD_AGW_PUBLIC_KEY="$ENV{PROD_AGW_PUBLIC_KEY}")
add_definitions(-DPROD_S3_ENDPOINT="$ENV{PROD_S3_ENDPOINT}")

add_definitions(-DDEV_AGW_PUBLIC_KEY="$ENV{DEV_AGW_PUBLIC_KEY}")
add_definitions(-DDEV_AGW_ENDPOINT="$ENV{DEV_AGW_ENDPOINT}")
add_definitions(-DDEV_S3_ENDPOINT="$ENV{DEV_S3_ENDPOINT}")

add_definitions(-DFREE_V2_ENDPOINT="$ENV{FREE_V2_ENDPOINT}")
add_definitions(-DPREM_V1_ENDPOINT="$ENV{PREM_V1_ENDPOINT}")

if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    set(PACKAGES ${PACKAGES} Widgets)
endif()

if(WIN32 OR (APPLE AND NOT IOS AND NOT MACOS_NE) OR (LINUX AND NOT ANDROID))
    set(PACKAGES ${PACKAGES} RemoteObjects)
endif()

# Qt 6.11 iOS kit may expose Darwin permission plugin packages whose objects are
# built for iphoneos only. In UI-only simulator builds they break linker input.
if(IOS AND IOS_SIMULATOR_UI_ONLY)
    foreach(_qt_perm_pkg IN ITEMS
        Qt6QDarwinCameraPermissionPlugin
        Qt6QDarwinMicrophonePermissionPlugin
        Qt6QDarwinBluetoothPermissionPlugin
        Qt6QDarwinCalendarPermissionPlugin
        Qt6QDarwinContactsPermissionPlugin
        Qt6QDarwinLocationPermissionPlugin
        Qt6QDarwinMediaLibraryPermissionPlugin
        Qt6QDarwinPhotoLibraryPermissionPlugin
        Qt6QDarwinReminderPermissionPlugin
        Qt6QDarwinRemindersPermissionPlugin
    )
        set(CMAKE_DISABLE_FIND_PACKAGE_${_qt_perm_pkg} TRUE)
    endforeach()
endif()

find_package(Qt6 REQUIRED COMPONENTS ${PACKAGES})

set(LIBS ${LIBS}
    Qt6::Core Qt6::Gui
    Qt6::Network Qt6::Xml
    Qt6::Quick Qt6::Svg Qt6::QuickControls2
    Qt6::Concurrent
)

if(NOT IOS)
    set(LIBS ${LIBS} Qt6::Core5Compat)
endif()

if(WIN32 OR (APPLE AND NOT IOS AND NOT MACOS_NE) OR (LINUX AND NOT ANDROID))
    set(LIBS ${LIBS} Qt6::RemoteObjects)
endif()

if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    set(LIBS ${LIBS} Qt6::Widgets)
endif()

qt_standard_project_setup()
qt_add_executable(${PROJECT} MANUAL_FINALIZATION)
target_include_directories(${PROJECT} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

if(WIN32 OR (APPLE AND NOT IOS AND NOT MACOS_NE) OR (LINUX AND NOT ANDROID))
    qt_add_repc_replicas(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc_interface.rep)
    qt_add_repc_replicas(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc_process_interface.rep)
endif()

qt6_add_resources(QRC ${QRC} ${CMAKE_CURRENT_LIST_DIR}/resources.qrc)

# -- i18n begin
set(CMAKE_AUTORCC ON)

set(AMNEZIAVPN_TS_FILES
    ${CMAKE_CURRENT_LIST_DIR}/translations/FBLink_ru_RU.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/FBLink_zh_CN.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/FBLink_fa_IR.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/FBLink_ar_EG.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/FBLink_my_MM.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/FBLink_uk_UA.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/FBLink_ur_PK.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/FBLink_hi_IN.ts
)

file(GLOB_RECURSE AMNEZIAVPN_TS_SOURCES *.qrc *.cpp *.h *.ui)

qt_create_translation(AMNEZIAVPN_QM_FILES ${AMNEZIAVPN_TS_SOURCES} ${AMNEZIAVPN_TS_FILES})

set(QM_FILE_LIST "")
foreach(FILE ${AMNEZIAVPN_QM_FILES})
    get_filename_component(QM_FILE_NAME ${FILE} NAME)
    list(APPEND QM_FILE_LIST "<file>${QM_FILE_NAME}</file>")
endforeach()
string(REPLACE ";" "" QM_FILE_LIST ${QM_FILE_LIST})

configure_file(${CMAKE_CURRENT_LIST_DIR}/translations/translations.qrc.in ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
qt6_add_resources(QRC ${I18NQRC} ${CMAKE_CURRENT_BINARY_DIR}/translations.qrc)
# -- i18n end

set(IS_CI ${CI})
if(IS_CI)
    message("Detected CI env")
    find_program(CCACHE "ccache")
    if(CCACHE)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE}")
    endif()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/3rdparty.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/cmake/sources.cmake)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../ipc
    ${CMAKE_CURRENT_LIST_DIR}/../common/logger
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(MACOS_NE)
    message("MACOS_NE is ON")
    add_definitions(-DQ_OS_MAC)
    add_definitions(-DMACOS_NE)
    message("Add macros for MacOS Network Extension")
else()
    message("MACOS_NE is OFF")
endif()

include_directories(mozilla)
include_directories(mozilla/shared)
include_directories(mozilla/models)

configure_file(${CMAKE_CURRENT_LIST_DIR}/../version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT} PRIVATE "MZ_DEBUG")
endif()

if(WIN32)
    configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/platforms/windows/FBLink.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/fblink.rc
    )

    set(LIBS ${LIBS}
        user32
        rasapi32
        shlwapi
        iphlpapi
        ws2_32
        gdi32
    )

    if(MSVC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    else()
        # MinGW: -mwindows sets Windows subsystem (no console window), equivalent to /SUBSYSTEM:WINDOWS
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
    endif()
endif()

if(APPLE)
    cmake_policy(SET CMP0099 NEW)
    cmake_policy(SET CMP0114 NEW)

    if(NOT BUILD_OSX_APP_IDENTIFIER)
        set(BUILD_OSX_APP_IDENTIFIER com.fblink.vpn CACHE STRING "OSX Application identifier")
    endif()
    if(NOT BUILD_IOS_APP_IDENTIFIER)
        set(BUILD_IOS_APP_IDENTIFIER com.fblink.vpn CACHE STRING "iOS Application identifier")
    endif()
    if(NOT BUILD_IOS_GROUP_IDENTIFIER)
        set(BUILD_IOS_GROUP_IDENTIFIER group.com.fblink.vpn CACHE STRING "iOS Group identifier")
    endif()
    if(NOT BUILD_VPN_DEVELOPMENT_TEAM)
        set(BUILD_VPN_DEVELOPMENT_TEAM X7UJ388FXK CACHE STRING "FBLink Development Team")
    endif()

    set(CMAKE_XCODE_GENERATE_SCHEME FALSE)
    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${BUILD_VPN_DEVELOPMENT_TEAM})
    set(CMAKE_XCODE_ATTRIBUTE_GROUP_ID_IOS ${BUILD_IOS_GROUP_IDENTIFIER})
endif()

if(LINUX AND NOT ANDROID)
    set(LIBS ${LIBS} -static-libstdc++ -static-libgcc -ldl)
    link_directories(${CMAKE_CURRENT_LIST_DIR}/platforms/linux)
endif()

if(WIN32 OR (APPLE AND NOT IOS AND NOT MACOS_NE) OR (LINUX AND NOT ANDROID))
    add_compile_definitions(AMNEZIA_DESKTOP)
endif()

if(ANDROID)
    include(cmake/android.cmake)
endif()

if(IOS)
    include(cmake/ios.cmake)
    include(cmake/ios-arch-fixup.cmake)
elseif(APPLE AND MACOS_NE)
    include(cmake/macos_ne.cmake)
elseif(APPLE)
    include(cmake/osxtools.cmake)
    include(cmake/macos.cmake)
endif()

list(REMOVE_DUPLICATES SOURCES)
list(REMOVE_DUPLICATES HEADERS)
list(REMOVE_DUPLICATES RESOURCES)
list(REMOVE_DUPLICATES QRC)
list(REMOVE_DUPLICATES I18NQRC)

target_link_libraries(${PROJECT} PRIVATE ${LIBS})
target_compile_definitions(${PROJECT} PRIVATE "MZ_$<UPPER_CASE:${MZ_PLATFORM_NAME}>")
if(IOS AND IOS_SIMULATOR_UI_ONLY)
    target_compile_definitions(${PROJECT} PRIVATE QT_NO_PERMISSIONS)
endif()

# iOS UI-only simulator build: avoid default static plugin set from Qt.
# In Qt 6.11 the default set may include Darwin permission init objects built
# for iphoneos, which breaks simulator linking.
if(IOS AND IOS_SIMULATOR_UI_ONLY AND COMMAND qt_import_plugins)
    qt_import_plugins(${PROJECT}
        NO_DEFAULT
        EXCLUDE_BY_TYPE permissions
        EXCLUDE_BY_TYPE qmltooling
    )

    function(_fblink_append_plugin_if_exists _out_var _name)
        if(TARGET Qt6::${_name})
            list(APPEND ${_out_var} Qt6::${_name})
            set(${_out_var} "${${_out_var}}" PARENT_SCOPE)
        elseif(TARGET Qt::${_name})
            list(APPEND ${_out_var} Qt::${_name})
            set(${_out_var} "${${_out_var}}" PARENT_SCOPE)
        endif()
    endfunction()

    set(_ios_platform_plugins "")
    _fblink_append_plugin_if_exists(_ios_platform_plugins QIOSIntegrationPlugin)

    set(_ios_image_plugins "")
    _fblink_append_plugin_if_exists(_ios_image_plugins QGifPlugin)
    _fblink_append_plugin_if_exists(_ios_image_plugins QICOPlugin)
    _fblink_append_plugin_if_exists(_ios_image_plugins QJpegPlugin)
    _fblink_append_plugin_if_exists(_ios_image_plugins QSvgPlugin)

    set(_ios_icon_plugins "")
    _fblink_append_plugin_if_exists(_ios_icon_plugins QSvgIconPlugin)

    set(_ios_networkinfo_plugins "")
    _fblink_append_plugin_if_exists(_ios_networkinfo_plugins QAppleNetworkInformationPlugin)

    set(_ios_tls_plugins "")
    _fblink_append_plugin_if_exists(_ios_tls_plugins QSecureTransportBackendPlugin)

    if(_ios_platform_plugins)
        qt_import_plugins(${PROJECT} INCLUDE_BY_TYPE platforms ${_ios_platform_plugins})
    endif()
    if(_ios_image_plugins)
        qt_import_plugins(${PROJECT} INCLUDE_BY_TYPE imageformats ${_ios_image_plugins})
    endif()
    if(_ios_icon_plugins)
        qt_import_plugins(${PROJECT} INCLUDE_BY_TYPE iconengines ${_ios_icon_plugins})
    endif()
    if(_ios_networkinfo_plugins)
        qt_import_plugins(${PROJECT} INCLUDE_BY_TYPE networkinformation ${_ios_networkinfo_plugins})
    endif()
    if(_ios_tls_plugins)
        qt_import_plugins(${PROJECT} INCLUDE_BY_TYPE tls ${_ios_tls_plugins})
    endif()
endif()

# deploy artifacts required to run the application to the debug build folder
if(WIN32)
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        set(DEPLOY_PLATFORM_PATH "windows/x64")
    else()
        set(DEPLOY_PLATFORM_PATH "windows/x32")
    endif()
elseif(LINUX)
    set(DEPLOY_PLATFORM_PATH "linux/client")
elseif(APPLE AND NOT IOS)
    set(DEPLOY_PLATFORM_PATH "macos")
endif()

if(NOT IOS AND NOT ANDROID AND NOT MACOS_NE)
    add_custom_command(
        TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/deploy/data/${DEPLOY_PLATFORM_PATH}
        $<TARGET_FILE_DIR:${PROJECT}>
        COMMAND_EXPAND_LISTS
    )
    add_custom_command(
        TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/client/3rd-prebuilt/deploy-prebuilt/${DEPLOY_PLATFORM_PATH}
        $<TARGET_FILE_DIR:${PROJECT}>
        COMMAND_EXPAND_LISTS
    )
endif()

target_sources(${PROJECT} PRIVATE ${SOURCES} ${HEADERS} ${RESOURCES} ${QRC} ${I18NQRC})

if(IOS AND IOS_SIMULATOR_UI_ONLY AND CMAKE_GENERATOR STREQUAL "Xcode")
    # Work around Qt 6.11 iOS package mixing device-only permission plugin
    # objects into simulator link lists.
    add_custom_command(TARGET ${PROJECT} PRE_LINK
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_LIST_DIR}/cmake/ios-filter-linkfile.cmake"
        VERBATIM
    )
endif()

if(IOS AND IOS_SIMULATOR_UI_ONLY AND COMMAND qt_set_finalizer_mode)
    # Avoid direct linking of static plugin initializer object libraries in
    # simulator UI-only mode. Qt 6.11 can otherwise inject iOS-only permission
    # plugin init objects (e.g. QDarwinCameraPermissionPlugin_init.cpp.o).
    qt_set_finalizer_mode(${PROJECT} DISABLE MODES static_plugins)
endif()

# Finalize the executable so Qt can gather/deploy QML modules and plugins correctly.
# On iOS, qt_finalize_executable() already handles QML plugin import and a manual
# qt_import_qml_plugins() call can produce duplicate static plugin symbols.
if(NOT IOS AND COMMAND qt_import_qml_plugins)
    qt_import_qml_plugins(${PROJECT})
endif()
if(COMMAND qt_finalize_executable)
    qt_finalize_executable(${PROJECT})
else()
    qt_finalize_target(${PROJECT})
endif()
