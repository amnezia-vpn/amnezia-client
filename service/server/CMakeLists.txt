cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT AmneziaVPN-service)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS DBus Core Network Widgets RemoteObjects Core5Compat)
qt_standard_project_setup()


configure_file(${CMAKE_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

set(HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/../../client/utilities.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/core/networkUtilities.h
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipc.h
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipcserver.h
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipcserverprocess.h
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipctun2socksprocess.h
    ${CMAKE_CURRENT_LIST_DIR}/localserver.h
    ${CMAKE_CURRENT_LIST_DIR}/../../common/logger/logger.h
    ${CMAKE_CURRENT_LIST_DIR}/router.h
    ${CMAKE_CURRENT_LIST_DIR}/systemservice.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../client/utilities.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../client/core/networkUtilities.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipcserver.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipcserverprocess.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipctun2socksprocess.cpp
    ${CMAKE_CURRENT_LIST_DIR}/localserver.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/logger/logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/router.cpp
    ${CMAKE_CURRENT_LIST_DIR}/systemservice.cpp
)

# Mozilla headres
set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/dnsutils.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/iputils.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/interfaceconfig.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/wireguardutils.h

    ${CMAKE_CURRENT_LIST_DIR}/../../client/platforms/dummy/dummynetworkwatcher.h

    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/shared/ipaddress.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/shared/loglevel.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/shared/leakdetector.h

    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/models/server.h

    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/controllerimpl.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/dnspingsender.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/localsocketcontroller.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/networkwatcher.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/networkwatcherimpl.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/pinghelper.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/pingsender.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/pingsenderfactory.h
)

include_directories(../../client/mozilla)
include_directories(../../client/mozilla/shared)
include_directories(../../client/mozilla/models)
include_directories(../../client/platforms/)

# Mozilla sources
set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/models/server.cpp

    ${CMAKE_CURRENT_LIST_DIR}/../../client/platforms/dummy/dummynetworkwatcher.cpp
    
    ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/interfaceconfig.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/shared/ipaddress.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/shared/leakdetector.cpp

    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/dnspingsender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/localsocketcontroller.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/networkwatcher.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/pinghelper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/pingsender.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/pingsenderfactory.cpp
)

if(UNIX)
    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/shared/signalhandler.h
    )
    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/../../client/mozilla/shared/signalhandler.cpp
    )
endif()

if (WIN32 OR APPLE OR LINUX)
    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/daemon.h
        ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/daemonlocalserver.h
        ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/daemonlocalserverconnection.h
    )
    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/daemon.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/daemonlocalserver.cpp
        ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/daemonlocalserverconnection.cpp
    )
endif()

if(WIN32)
    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/tapcontroller_win.h
        ${CMAKE_CURRENT_LIST_DIR}/router_win.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowsdaemon.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowsdaemontunnel.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowsfirewall.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/windowscommons.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/windowsservicemanager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/dnsutilswindows.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowssplittunnel.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowstunnelservice.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/wireguardutilswindows.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowstunnellogger.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowsroutemonitor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/windowsutils.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/windowspingsender.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/windowsnetworkwatcher.h
        ${CMAKE_CURRENT_LIST_DIR}/../../client/daemon/daemonerrors.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/tapcontroller_win.cpp
        ${CMAKE_CURRENT_LIST_DIR}/router_win.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowsdaemon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowsdaemontunnel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowsfirewall.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/windowscommons.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/windowsservicemanager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/dnsutilswindows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowssplittunnel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowstunnelservice.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/wireguardutilswindows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowstunnellogger.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/daemon/windowsroutemonitor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/windowspingsender.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/windowsnetworkwatcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/windows/windowsutils.cpp
    )

    set(LIBS
        user32
        rasapi32
        shlwapi
        iphlpapi
        ws2_32
        iphlpapi
        gdi32
        Advapi32
        Kernel32
    )

    add_compile_definitions(_WINSOCKAPI_)
endif()

if(APPLE)
    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/helper_route_mac.h
        ${CMAKE_CURRENT_LIST_DIR}/router_mac.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/helper_route_mac.c
        ${CMAKE_CURRENT_LIST_DIR}/router_mac.cpp
    )

    #Mozilla
    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/ios/iosnetworkwatcher.h

        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/macosnetworkwatcher.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/macospingsender.h

        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/dnsutilsmacos.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/iputilsmacos.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/macosdaemon.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/macosroutemonitor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/wireguardutilsmacos.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/macosfirewall.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/ios/iosnetworkwatcher.mm

        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/macosnetworkwatcher.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/macospingsender.cpp

        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/dnsutilsmacos.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/iputilsmacos.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/macosdaemon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/macosroutemonitor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/wireguardutilsmacos.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/macos/daemon/macosfirewall.cpp
    )
endif()

if(LINUX)

    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/router_linux.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxnetworkwatcher.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxnetworkwatcherworker.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxdependencies.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/iputilslinux.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/dbustypeslinux.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/linuxdaemon.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/dnsutilslinux.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/wireguardutilslinux.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/linuxroutemonitor.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/linuxfirewall.h        
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/router_linux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxnetworkwatcher.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxnetworkwatcherworker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxdependencies.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/dnsutilslinux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/iputilslinux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/linuxdaemon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/wireguardutilslinux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/linuxroutemonitor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/linuxfirewall.cpp
    )
endif()

include(${CMAKE_CURRENT_LIST_DIR}/../src/qtservice.cmake)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../client
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc
    ${CMAKE_CURRENT_LIST_DIR}/../../common/logger
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(${PROJECT} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT} PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::RemoteObjects Qt6::Core5Compat Qt6::DBus ${LIBS})
target_compile_definitions(${PROJECT} PRIVATE "MZ_$<UPPER_CASE:${MZ_PLATFORM_NAME}>")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT} PRIVATE "MZ_DEBUG")
endif()

if(APPLE)
    if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        set_target_properties(${PROJECT} PROPERTIES
            INSTALL_RPATH "@executable_path/../Frameworks"
            BUILD_WITH_INSTALL_RPATH TRUE
        )
    endif()

    find_library(FW_COREFOUNDATION CoreFoundation)
    find_library(FW_SYSTEMCONFIG SystemConfiguration)
    find_library(FW_SERVICEMGMT ServiceManagement)
    find_library(FW_SECURITY Security)
    find_library(FW_COREWLAN CoreWLAN)
    find_library(FW_NETWORK Network)
    find_library(FW_USER_NOTIFICATIONS UserNotifications)

    target_link_libraries(${PROJECT} PRIVATE ${FW_COREFOUNDATION})
    target_link_libraries(${PROJECT} PRIVATE ${FW_SYSTEMCONFIG})
    target_link_libraries(${PROJECT} PRIVATE ${FW_SERVICEMGMT})
    target_link_libraries(${PROJECT} PRIVATE ${FW_SECURITY})
    target_link_libraries(${PROJECT} PRIVATE ${FW_COREWLAN})
    target_link_libraries(${PROJECT} PRIVATE ${FW_NETWORK})
    target_link_libraries(${PROJECT} PRIVATE ${FW_USER_NOTIFICATIONS})


endif()

qt_add_repc_sources(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipc_interface.rep)
qt_add_repc_sources(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipc_process_interface.rep)
qt_add_repc_sources(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipc_process_tun2socks.rep)

# copy deploy artifacts required to run the application to the debug build folder
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
