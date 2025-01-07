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

add_definitions(-DDEV_AGW_PUBLIC_KEY="$ENV{DEV_AGW_PUBLIC_KEY}")
add_definitions(-DDEV_AGW_ENDPOINT="$ENV{DEV_AGW_ENDPOINT}")
add_definitions(-DDEV_S3_ENDPOINT="$ENV{DEV_S3_ENDPOINT}")

if(IOS)
    set(PACKAGES ${PACKAGES} Multimedia)
endif()

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

if(IOS)
    set(LIBS ${LIBS} Qt6::Multimedia)
endif()

if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    set(LIBS ${LIBS} Qt6::Widgets)
endif()

qt_standard_project_setup()
qt_add_executable(${PROJECT} MANUAL_FINALIZATION)

if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    qt_add_repc_replicas(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc_interface.rep)
    qt_add_repc_replicas(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc_process_interface.rep)
    qt_add_repc_replicas(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc_process_tun2socks.rep)
endif()

qt6_add_resources(QRC ${QRC} ${CMAKE_CURRENT_LIST_DIR}/resources.qrc)

# -- i18n begin
set(CMAKE_AUTORCC ON)

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

if(IOS)
    execute_process(COMMAND bash ${CMAKE_CURRENT_LIST_DIR}/ios/scripts/openvpn.sh args
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
endif()

set(IS_CI ${CI})
if(IS_CI)
    message("Detected CI env")
    find_program(CCACHE "ccache")
    if(CCACHE)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE}")
    endif()
endif()


include(${CMAKE_CURRENT_LIST_DIR}/cmake/3rdparty.cmake)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/../ipc
    ${CMAKE_CURRENT_LIST_DIR}/../common/logger
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file(${CMAKE_CURRENT_LIST_DIR}/../version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/migrations.h
    ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc.h
    ${CMAKE_CURRENT_LIST_DIR}/amnezia_application.h
    ${CMAKE_CURRENT_LIST_DIR}/containers/containers_defs.h
    ${CMAKE_CURRENT_LIST_DIR}/core/defs.h
    ${CMAKE_CURRENT_LIST_DIR}/core/errorstrings.h
    ${CMAKE_CURRENT_LIST_DIR}/core/scripts_registry.h
    ${CMAKE_CURRENT_LIST_DIR}/core/server_defs.h
    ${CMAKE_CURRENT_LIST_DIR}/core/controllers/apiController.h
    ${CMAKE_CURRENT_LIST_DIR}/core/controllers/serverController.h
    ${CMAKE_CURRENT_LIST_DIR}/core/controllers/vpnConfigurationController.h
    ${CMAKE_CURRENT_LIST_DIR}/protocols/protocols_defs.h
    ${CMAKE_CURRENT_LIST_DIR}/protocols/qml_register_protocols.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/qautostart.h
    ${CMAKE_CURRENT_LIST_DIR}/protocols/vpnprotocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    ${CMAKE_CURRENT_LIST_DIR}/core/sshclient.h
    ${CMAKE_CURRENT_LIST_DIR}/core/networkUtilities.h
    ${CMAKE_CURRENT_LIST_DIR}/core/serialization/serialization.h
    ${CMAKE_CURRENT_LIST_DIR}/core/serialization/transfer.h
    ${CMAKE_CURRENT_LIST_DIR}/core/enums/apiEnums.h
    ${CMAKE_CURRENT_LIST_DIR}/../common/logger/logger.h
    ${CMAKE_CURRENT_LIST_DIR}/utils/qmlUtils.h
)

# Mozilla headres
set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/mozilla/models/server.h
    ${CMAKE_CURRENT_LIST_DIR}/mozilla/shared/ipaddress.h
    ${CMAKE_CURRENT_LIST_DIR}/mozilla/shared/leakdetector.h
    ${CMAKE_CURRENT_LIST_DIR}/mozilla/controllerimpl.h
    ${CMAKE_CURRENT_LIST_DIR}/mozilla/localsocketcontroller.h
)

include_directories(mozilla)
include_directories(mozilla/shared)
include_directories(mozilla/models)

if(NOT IOS)
    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/QRCodeReaderBase.h
    )
endif()

if(NOT ANDROID)
    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/ui/notificationhandler.h
    )
endif()

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/migrations.cpp
    ${CMAKE_CURRENT_LIST_DIR}/amnezia_application.cpp
    ${CMAKE_CURRENT_LIST_DIR}/containers/containers_defs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/errorstrings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/scripts_registry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/server_defs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/controllers/apiController.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/controllers/serverController.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/controllers/vpnConfigurationController.cpp
    ${CMAKE_CURRENT_LIST_DIR}/protocols/protocols_defs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/qautostart.cpp
    ${CMAKE_CURRENT_LIST_DIR}/protocols/vpnprotocol.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/sshclient.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/networkUtilities.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/serialization/outbound.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/serialization/inbound.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/serialization/ss.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/serialization/ssd.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/serialization/vless.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/serialization/trojan.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/serialization/vmess.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/serialization/vmess_new.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../common/logger/logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/utils/qmlUtils.cpp
)

# Mozilla sources
set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/mozilla/models/server.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mozilla/shared/ipaddress.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mozilla/shared/leakdetector.cpp
    ${CMAKE_CURRENT_LIST_DIR}/mozilla/localsocketcontroller.cpp
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT} PRIVATE "MZ_DEBUG")
endif()

if(NOT IOS)
    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/QRCodeReaderBase.cpp
    )
endif()

if(NOT ANDROID)
    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/ui/notificationhandler.cpp
    )
endif()

file(GLOB COMMON_FILES_H CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.h)
file(GLOB COMMON_FILES_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/*.cpp)

file(GLOB_RECURSE PAGE_LOGIC_H CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/*.h)
file(GLOB_RECURSE PAGE_LOGIC_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/*.cpp)

file(GLOB CONFIGURATORS_H CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/configurators/*.h)
file(GLOB CONFIGURATORS_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/configurators/*.cpp)

file(GLOB UI_MODELS_H CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/*.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/protocols/*.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/services/*.h
)
file(GLOB UI_MODELS_CPP CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/protocols/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/services/*.cpp
)

file(GLOB UI_CONTROLLERS_H CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ui/controllers/*.h)
file(GLOB UI_CONTROLLERS_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/ui/controllers/*.cpp)

set(HEADERS ${HEADERS}
    ${COMMON_FILES_H}
    ${PAGE_LOGIC_H}
    ${CONFIGURATORS_H}
    ${UI_MODELS_H}
    ${UI_CONTROLLERS_H}
)
set(SOURCES ${SOURCES}
    ${COMMON_FILES_CPP}
    ${PAGE_LOGIC_CPP}
    ${CONFIGURATORS_CPP}
    ${UI_MODELS_CPP}
    ${UI_CONTROLLERS_CPP}
)

if(WIN32)
    configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/platforms/windows/amneziavpn.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/amneziavpn.rc
    )

    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/protocols/ikev2_vpn_protocol_windows.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/protocols/ikev2_vpn_protocol_windows.cpp
    )

    set(RESOURCES ${RESOURCES}
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
    cmake_policy(SET CMP0099 OLD)
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

endif()

if(LINUX AND NOT ANDROID)
    set(LIBS ${LIBS} -static-libstdc++ -static-libgcc -ldl)
    link_directories(${CMAKE_CURRENT_LIST_DIR}/platforms/linux)
endif()

if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    message("Client desktop build")
    add_compile_definitions(AMNEZIA_DESKTOP)

    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/core/ipcclient.h
        ${CMAKE_CURRENT_LIST_DIR}/core/privileged_process.h
        ${CMAKE_CURRENT_LIST_DIR}/ui/systemtray_notificationhandler.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/openvpnprotocol.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/openvpnovercloakprotocol.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/shadowsocksvpnprotocol.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/wireguardprotocol.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/xrayprotocol.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/awgprotocol.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/core/ipcclient.cpp
        ${CMAKE_CURRENT_LIST_DIR}/core/privileged_process.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ui/systemtray_notificationhandler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/openvpnprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/openvpnovercloakprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/shadowsocksvpnprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/wireguardprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/xrayprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/awgprotocol.cpp
    )
endif()

if(ANDROID)
    include(cmake/android.cmake)
endif()

if(IOS)
    include(cmake/ios.cmake)
    include(cmake/ios-arch-fixup.cmake)
elseif(APPLE AND NOT IOS)
    include(cmake/osxtools.cmake)
    include(cmake/macos.cmake)
endif()

target_link_libraries(${PROJECT} PRIVATE ${LIBS})
target_compile_definitions(${PROJECT} PRIVATE "MZ_$<UPPER_CASE:${MZ_PLATFORM_NAME}>")

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

if(NOT IOS AND NOT ANDROID)
    add_custom_command(
        TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,copy_directory,true>
        ${CMAKE_SOURCE_DIR}/deploy/data/${DEPLOY_PLATFORM_PATH}
        $<TARGET_FILE_DIR:${PROJECT}>
        COMMAND_EXPAND_LISTS
    )
    add_custom_command(
        TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E $<IF:$<CONFIG:Debug>,copy_directory,true>
        ${CMAKE_SOURCE_DIR}/client/3rd-prebuilt/deploy-prebuilt/${DEPLOY_PLATFORM_PATH}
        $<TARGET_FILE_DIR:${PROJECT}>
        COMMAND_EXPAND_LISTS
    )

endif()

target_sources(${PROJECT} PRIVATE ${SOURCES} ${HEADERS} ${RESOURCES} ${QRC} ${I18NQRC})
qt_finalize_target(${PROJECT})
