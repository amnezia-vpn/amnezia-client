cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT AmneziaVPN)
project(${PROJECT})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY AUTOGEN_TARGETS_FOLDER "Autogen")
set_property(GLOBAL PROPERTY AUTOMOC_TARGETS_FOLDER "Autogen")
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "Autogen")

set(PACKAGES
    Core Gui Network Xml
    RemoteObjects Quick Svg QuickControls2
    Core5Compat Concurrent LinguistTools
)

execute_process(
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMAND git rev-parse --short HEAD
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_definitions(-DGIT_COMMIT_HASH="${GIT_COMMIT_HASH}")

add_definitions(-DPROD_AGW_PUBLIC_KEY="$ENV{PROD_AGW_PUBLIC_KEY}")
add_definitions(-DPROD_S3_ENDPOINT="$ENV{PROD_S3_ENDPOINT}")
add_definitions(-DFALLBACK_S3_ENDPOINT="$ENV{FALLBACK_S3_ENDPOINT}")

add_definitions(-DDEV_AGW_PUBLIC_KEY="$ENV{DEV_AGW_PUBLIC_KEY}")
add_definitions(-DDEV_AGW_ENDPOINT="$ENV{DEV_AGW_ENDPOINT}")
add_definitions(-DDEV_S3_ENDPOINT="$ENV{DEV_S3_ENDPOINT}")

add_definitions(-DFREE_V2_ENDPOINT="$ENV{FREE_V2_ENDPOINT}")
add_definitions(-DPREM_V1_ENDPOINT="$ENV{PREM_V1_ENDPOINT}")

if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    set(PACKAGES ${PACKAGES} Widgets)
endif()

find_package(Qt6 REQUIRED COMPONENTS ${PACKAGES})

set(LIBS ${LIBS}
    Qt6::Core Qt6::Gui
    Qt6::Network Qt6::Xml Qt6::RemoteObjects
    Qt6::Quick Qt6::Svg Qt6::QuickControls2
    Qt6::Core5Compat Qt6::Concurrent
)

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

qt6_add_resources(QRC ${QRC}
    ${CMAKE_CURRENT_LIST_DIR}/images/images.qrc
    ${CMAKE_CURRENT_LIST_DIR}/images/flagKit.qrc
    ${CMAKE_CURRENT_LIST_DIR}/client_scripts/clientScripts.qrc
    ${CMAKE_CURRENT_LIST_DIR}/ui/qml/qml.qrc
    ${CMAKE_CURRENT_LIST_DIR}/server_scripts/serverScripts.qrc
)

# -- i18n begin
set(AMNEZIAVPN_TS_FILES
    ${CMAKE_CURRENT_LIST_DIR}/translations/amneziavpn_ru_RU.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/amneziavpn_zh_CN.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/amneziavpn_fa_IR.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/amneziavpn_ar_EG.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/amneziavpn_my_MM.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/amneziavpn_uk_UA.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/amneziavpn_ur_PK.ts
    ${CMAKE_CURRENT_LIST_DIR}/translations/amneziavpn_hi_IN.ts
)

qt6_add_translations(${PROJECT}
    TS_FILES ${AMNEZIAVPN_TS_FILES}
    RESOURCE_PREFIX "/translations"
)
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
        ${CMAKE_CURRENT_LIST_DIR}/platforms/windows/amneziavpn.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/amneziavpn.rc
    )

    set(LIBS ${LIBS}
        user32
        rasapi32
        shlwapi
        iphlpapi
        ws2_32
        gdi32
    )

    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()

if(APPLE)
    cmake_policy(SET CMP0099 NEW)
    cmake_policy(SET CMP0114 NEW)

    if(NOT BUILD_OSX_APP_IDENTIFIER)
        set(BUILD_OSX_APP_IDENTIFIER org.amnezia.AmneziaVPN CACHE STRING "OSX Application identifier")
    endif()
    if(NOT BUILD_IOS_APP_IDENTIFIER)
        set(BUILD_IOS_APP_IDENTIFIER org.amnezia.AmneziaVPN CACHE STRING "iOS Application identifier")
    endif()
    if(NOT BUILD_IOS_GROUP_IDENTIFIER)
        set(BUILD_IOS_GROUP_IDENTIFIER group.org.amnezia.AmneziaVPN CACHE STRING "iOS Group identifier")
    endif()
    if(NOT BUILD_VPN_DEVELOPMENT_TEAM)
        set(BUILD_VPN_DEVELOPMENT_TEAM X7UJ388FXK CACHE STRING "Amnezia VPN Development Team")
    endif()

    set(CMAKE_XCODE_GENERATE_SCHEME FALSE)
    set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${BUILD_VPN_DEVELOPMENT_TEAM})
    set(CMAKE_XCODE_ATTRIBUTE_GROUP_ID_IOS ${BUILD_IOS_GROUP_IDENTIFIER})
    
    if (BUILD_VPN_KEYCHAIN)
        set(CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--keychain ${BUILD_VPN_KEYCHAIN}")
    endif()
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

list(APPEND SOURCES ${CMAKE_CURRENT_LIST_DIR}/main.cpp)

target_link_libraries(${PROJECT} PRIVATE ${LIBS})
target_compile_definitions(${PROJECT} PRIVATE "MZ_$<UPPER_CASE:${MZ_PLATFORM_NAME}>")

target_sources(${PROJECT} PRIVATE ${SOURCES} ${HEADERS} ${RESOURCES} ${QRC} ${I18NQRC})

# Finalize the executable so Qt can gather/deploy QML modules and plugins correctly (Android needs this).
if(COMMAND qt_import_qml_plugins)
    qt_import_qml_plugins(${PROJECT})
endif()
if(COMMAND qt_finalize_executable)
    qt_finalize_executable(${PROJECT})
else()
    qt_finalize_target(${PROJECT})
endif()

install(TARGETS ${PROJECT}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    RUNTIME_DEPENDENCY_SET client_deps
    COMPONENT AmneziaVPN
)

if(APPLE)
    set(RUNTIME_DEPS_DIR ${CMAKE_INSTALL_BINDIR}/AmneziaVPN.app/Contents/Frameworks)
else()
    set(RUNTIME_DEPS_DIR ${CMAKE_INSTALL_BINDIR})
endif()

install(RUNTIME_DEPENDENCY_SET client_deps
    PRE_EXCLUDE_REGEXES
        [[api-ms-win-.*]]
        [[ext-ms-.*]]
        [[kernel32\.dll]]
        [[hvsifiletrust\.dll]]
        [[libc\.so\..*]] [[libgcc_s\.so\..*]] [[libm\.so\..*]] [[libstdc\+\+\.so\..*]]
        [[.*\.framework]]
        [[^[Qq]t.*]]
    POST_EXCLUDE_REGEXES
        [[^.*[\\/]system32[\\/].*\.dll$]]
        [[^/lib.*]]
        [[^/usr/lib.*]]
    DIRECTORIES ${CONAN_RUNTIME_LIB_DIRS}
    COMPONENT AmneziaVPN
    DESTINATION "${RUNTIME_DEPS_DIR}"
)

set(deploy_tool_options "")
if(WIN32)
    set(deploy_tool_options "--force-openssl --force")
endif()

qt_generate_deploy_qml_app_script(
    TARGET ${PROJECT}
    OUTPUT_SCRIPT QT_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_TOOL_OPTIONS ${deploy_tool_options}
)
install(SCRIPT ${QT_DEPLOY_SCRIPT}
    COMPONENT AmneziaVPN
)

if (APPLE AND NOT IOS AND NOT MACOS_NE)
    list(APPEND OVPN_SCRIPTS "${CMAKE_SOURCE_DIR}/deploy/data/macos/update-resolv-conf.sh")
endif()
if (LINUX AND NOT ANDROID)
    list(APPEND OVPN_SCRIPTS "${CMAKE_SOURCE_DIR}/deploy/data/linux/update-resolv-conf.sh")
endif()

if(OVPN_SCRIPTS)
    add_custom_command(TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OVPN_SCRIPTS}
        "$<TARGET_FILE_DIR:${PROJECT}>"
    )

    install(FILES ${OVPN_SCRIPTS}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT AmneziaVPN
        PERMISSIONS
            OWNER_READ OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
    )
endif()
