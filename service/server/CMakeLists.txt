cmake_minimum_required(VERSION 3.25.0 FATAL_ERROR)

set(PROJECT AmneziaVPN-service)
project(${PROJECT} VERSION ${AMNEZIAVPN_VERSION})

if (APPLE)
    # CMAKE_INSTALL_BINDIR must be called before find_package(Qt6) for
    # Qt to configure prefixes properly
    # FIXME: make Amnezia-service a bundle and remove this hack completely
    set(CMAKE_INSTALL_BINDIR "AmneziaVPN.app/Contents/MacOS")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS DBus Core Network Widgets RemoteObjects Core5Compat Concurrent)
qt_standard_project_setup()

configure_file(${CMAKE_SOURCE_DIR}/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

set(QSIMPLECRYPTO_DIR ${CMAKE_CURRENT_LIST_DIR}/../../client/3rd/QSimpleCrypto/src)

include_directories(
    ${QSIMPLECRYPTO_DIR}
)

set(HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/../../client/core/utils/utilities.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/secureQSettings.h
    ${CMAKE_CURRENT_LIST_DIR}/../../client/core/utils/networkUtilities.h
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipc.h
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipcserver.h
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipcserverprocess.h
    ${CMAKE_CURRENT_LIST_DIR}/localserver.h
    ${CMAKE_CURRENT_LIST_DIR}/../../common/logger/logger.h
    ${CMAKE_CURRENT_LIST_DIR}/router.h
    ${CMAKE_CURRENT_LIST_DIR}/killswitch.h
    ${CMAKE_CURRENT_LIST_DIR}/systemservice.h
    ${CMAKE_CURRENT_LIST_DIR}/xray.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    ${QSIMPLECRYPTO_DIR}/include/QAead.h
    ${QSIMPLECRYPTO_DIR}/include/QBlockCipher.h
    ${QSIMPLECRYPTO_DIR}/include/QRsa.h
    ${QSIMPLECRYPTO_DIR}/include/QSimpleCrypto_global.h
    ${QSIMPLECRYPTO_DIR}/include/QX509.h
    ${QSIMPLECRYPTO_DIR}/include/QX509Store.h
)

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../client/core/utils/utilities.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../client/secureQSettings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../client/core/utils/networkUtilities.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipcserver.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../ipc/ipcserverprocess.cpp
    ${CMAKE_CURRENT_LIST_DIR}/localserver.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/logger/logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/router.cpp
    ${CMAKE_CURRENT_LIST_DIR}/killswitch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/systemservice.cpp
    ${CMAKE_CURRENT_LIST_DIR}/xray.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QAead.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QBlockCipher.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QRsa.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QX509.cpp
    ${QSIMPLECRYPTO_DIR}/sources/QX509Store.cpp
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

set(RESOURCES)

if(WIN32)
    configure_file(
        ${CMAKE_CURRENT_LIST_DIR}/amneziavpn-service.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/amneziavpn-service.rc
    )

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

    set(RESOURCES ${RESOURCES}
        ${CMAKE_CURRENT_BINARY_DIR}/amneziavpn-service.rc
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
        qt6keychain
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

    set(LIBS
        resolv
        qt6keychain
    )

endif()

if(LINUX)

    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/router_linux.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxnetworkwatcher.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxnetworkwatcherworker.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxdependencies.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxpingsender.h 
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
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/linuxpingsender.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/dnsutilslinux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/iputilslinux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/linuxdaemon.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/wireguardutilslinux.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/linuxroutemonitor.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../client/platforms/linux/daemon/linuxfirewall.cpp
    )

    set(LIBS
        ${OPENSSL_LIB_CRYPTO_PATH}
        qt6keychain
        -static-libstdc++
        -static-libgcc
        -ldl
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


add_executable(${PROJECT} ${SOURCES} ${HEADERS} ${RESOURCES})
target_link_libraries(${PROJECT} PRIVATE Qt6::Core Qt6::Widgets Qt6::Network Qt6::RemoteObjects Qt6::Core5Compat Qt6::DBus Qt6::Concurrent ${LIBS})
target_compile_definitions(${PROJECT} PRIVATE "MZ_$<UPPER_CASE:${MZ_PLATFORM_NAME}>")

find_package(amnezia-xray-bindings REQUIRED)
target_link_libraries(${PROJECT} PRIVATE amnezia::xray-bindings)

find_package(OpenSSL REQUIRED)
target_link_libraries(${PROJECT} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${PROJECT} PRIVATE "MZ_DEBUG")
endif()

if(APPLE)
    set_target_properties(${PROJECT} PROPERTIES
        INSTALL_RPATH "@executable_path/../Frameworks"
    )

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

if(APPLE)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/deploy/data/macos/pf
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT AmneziaVPN
    )
endif()

# install non-linked dependencies
if(WIN32)
    find_package(awg-windows REQUIRED)
    list(APPEND CONAN_EXECS $<TARGET_FILE:amnezia::awg-windows>)

    find_package(wintun REQUIRED)
    list(APPEND CONAN_BINS $<TARGET_FILE:zx2c4::wintun>)
else()
    find_package(awg-go REQUIRED)
    list(APPEND CONAN_EXECS $<TARGET_FILE:amnezia::awg-go>)
endif()

find_package(openvpn REQUIRED)
list(APPEND CONAN_EXECS $<TARGET_FILE:openvpn::openvpn>)

find_package(tun2socks REQUIRED)
list(APPEND CONAN_EXECS $<TARGET_FILE:xjasonlyu::tun2socks>)

find_package(v2ray-rules-dat REQUIRED)
list(APPEND CONAN_BINS ${GEOSITE_DAT_PATH} ${GEOIP_DAT_PATH})

add_custom_command(TARGET ${PROJECT} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CONAN_EXECS} ${CONAN_BINS}
        "$<TARGET_FILE_DIR:${PROJECT}>"
)
if(WIN32)
    # using PERMISSIONS on Windows appends read-only flag
    # to the files so just omit it for it
    install(FILES ${CONAN_EXECS}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT AmneziaVPN
    )
else()
    install(FILES ${CONAN_EXECS}
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT AmneziaVPN
        PERMISSIONS
            OWNER_READ OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
    )
endif()
install(FILES ${CONAN_BINS}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT AmneziaVPN
)

# install drivers
if (WIN32)
    find_package(tap-windows6 REQUIRED)
    add_custom_command(TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${TAP_WINDOWS6_BIN}"
            "$<TARGET_FILE_DIR:${PROJECT}>/tap"
    )
    install(DIRECTORY "${TAP_WINDOWS6_BIN}/"
        DESTINATION "${CMAKE_INSTALL_BINDIR}/tap"
        COMPONENT AmneziaVPN
    )

    find_package(win-split-tunnel REQUIRED)
    add_custom_command(TARGET ${PROJECT} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${WIN_SPLIT_TUNNEL_BIN}"
            "$<TARGET_FILE_DIR:${PROJECT}>"
    )
    install(DIRECTORY "${WIN_SPLIT_TUNNEL_BIN}/"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT AmneziaVPN
    )
endif()

# install target
install(TARGETS ${PROJECT}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    RUNTIME_DEPENDENCY_SET service_deps
    COMPONENT AmneziaVPN
)

if(APPLE)
    set(RUNTIME_DEPS_DIR ${CMAKE_INSTALL_BINDIR}/../Frameworks)
else()
    set(RUNTIME_DEPS_DIR ${CMAKE_INSTALL_BINDIR})
endif()

install(RUNTIME_DEPENDENCY_SET service_deps
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

qt_generate_deploy_app_script(
    TARGET ${PROJECT}
    OUTPUT_SCRIPT QT_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${QT_DEPLOY_SCRIPT}
    COMPONENT AmneziaVPN
)
