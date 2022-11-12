cmake_minimum_required(VERSION 3.23.0 FATAL_ERROR)

set(PROJECT AmneziaVPN)
project(${PROJECT})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS 
    Widgets Core Gui Network Xml 
    RemoteObjects Quick Svg Quickcontrols2 
    Core5Compat
)
qt_standard_project_setup()

# # silent builds on CI env
# IS_CI=$$(CI)
# !isEmpty(IS_CI){
#   message("Detected CI env")
#   CONFIG += silent ccache
# }

include(${CMAKE_CURRENT_LIST_DIR}/3rd/QtSsh/src/ssh/qssh.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/3rd/QtSsh/src/botan/botan.cmake)
if(NOT IOS AND NOT ANDROID)
   include(${CMAKE_CURRENT_LIST_DIR}/3rd/SingleApplication/singleapplication.cmake)
endif()
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/3rd/SortFilterProxyModel)
set(LIBS ${LIBS} qt6keychain)

include(${CMAKE_CURRENT_LIST_DIR}/3rd/qrcodegen/qrcodegen.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/3rd/QSimpleCrypto/QSimpleCrypto.cmake)

set(BUILD_WITH_QT6 ON)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/3rd/qtkeychain)

# TODO it seems like i'm doing it wrong
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/3rd/QSimpleCrypto/include
    ${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/include
    ${CMAKE_CURRENT_LIST_DIR}/../ipc
    ${CMAKE_CURRENT_LIST_DIR}/3rd/qtkeychain
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/3rd/qtkeychain
)

set(HEADERS ${HEADERS}
    ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc.h
    ${CMAKE_CURRENT_LIST_DIR}/amnezia_application.h
    ${CMAKE_CURRENT_LIST_DIR}/configurators/cloak_configurator.h
    ${CMAKE_CURRENT_LIST_DIR}/configurators/configurator_base.h
    ${CMAKE_CURRENT_LIST_DIR}/configurators/ikev2_configurator.h
    ${CMAKE_CURRENT_LIST_DIR}/configurators/shadowsocks_configurator.h
    ${CMAKE_CURRENT_LIST_DIR}/configurators/ssh_configurator.h
    ${CMAKE_CURRENT_LIST_DIR}/configurators/vpn_configurator.h
    ${CMAKE_CURRENT_LIST_DIR}/configurators/wireguard_configurator.h
    ${CMAKE_CURRENT_LIST_DIR}/containers/containers_defs.h
    ${CMAKE_CURRENT_LIST_DIR}/core/defs.h
    ${CMAKE_CURRENT_LIST_DIR}/core/errorstrings.h
    ${CMAKE_CURRENT_LIST_DIR}/configurators/openvpn_configurator.h
    ${CMAKE_CURRENT_LIST_DIR}/core/scripts_registry.h
    ${CMAKE_CURRENT_LIST_DIR}/core/server_defs.h
    ${CMAKE_CURRENT_LIST_DIR}/core/servercontroller.h
    ${CMAKE_CURRENT_LIST_DIR}/debug.h
    ${CMAKE_CURRENT_LIST_DIR}/defines.h
    ${CMAKE_CURRENT_LIST_DIR}/managementserver.h
    ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/MobileUtils.h
    ${CMAKE_CURRENT_LIST_DIR}/platforms/linux/leakdetector.h
    ${CMAKE_CURRENT_LIST_DIR}/protocols/protocols_defs.h
    ${CMAKE_CURRENT_LIST_DIR}/secure_qsettings.h
    ${CMAKE_CURRENT_LIST_DIR}/settings.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/notificationhandler.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/containers_model.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/protocols_model.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/AppSettingsLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/GeneralSettingsLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/NetworkSettingsLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/NewServerProtocolsLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/PageLogicBase.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/QrDecoderLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ServerConfiguringProgressLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ServerContainersLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ServerListLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ServerSettingsLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ShareConnectionLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/SitesLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/StartPageLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ViewConfigLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/VpnLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/WizardLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/protocols/CloakLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/protocols/OpenVpnLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/protocols/OtherProtocolsLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/protocols/PageProtocolLogicBase.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/protocols/ShadowSocksLogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/property_helper.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/servers_model.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/uilogic.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/qautostart.h
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/sites_model.h
    ${CMAKE_CURRENT_LIST_DIR}/utilities.h
    ${CMAKE_CURRENT_LIST_DIR}/vpnconnection.h
    ${CMAKE_CURRENT_LIST_DIR}/protocols/vpnprotocol.h
    ${CMAKE_CURRENT_LIST_DIR}/logger.h
    ${CMAKE_CURRENT_LIST_DIR}/loghandler.h
    ${CMAKE_CURRENT_LIST_DIR}/loglevel.h
    ${CMAKE_CURRENT_LIST_DIR}/constants.h
    ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/QRCodeReaderBase.h
)

set(SOURCES ${SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/amnezia_application.cpp
    ${CMAKE_CURRENT_LIST_DIR}/configurators/cloak_configurator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/configurators/configurator_base.cpp
    ${CMAKE_CURRENT_LIST_DIR}/configurators/ikev2_configurator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/configurators/shadowsocks_configurator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/configurators/ssh_configurator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/configurators/vpn_configurator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/configurators/wireguard_configurator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/containers/containers_defs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/errorstrings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/configurators/openvpn_configurator.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/scripts_registry.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/server_defs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/core/servercontroller.cpp
    ${CMAKE_CURRENT_LIST_DIR}/debug.cpp
    ${CMAKE_CURRENT_LIST_DIR}/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/managementserver.cpp
    ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/MobileUtils.cpp
    ${CMAKE_CURRENT_LIST_DIR}/platforms/linux/leakdetector.cpp
    ${CMAKE_CURRENT_LIST_DIR}/protocols/protocols_defs.cpp
    ${CMAKE_CURRENT_LIST_DIR}/secure_qsettings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/settings.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/notificationhandler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/containers_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/protocols_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/AppSettingsLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/GeneralSettingsLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/NetworkSettingsLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/NewServerProtocolsLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/PageLogicBase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/QrDecoderLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ServerConfiguringProgressLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ServerContainersLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ServerListLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ServerSettingsLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ShareConnectionLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/SitesLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/StartPageLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/ViewConfigLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/VpnLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/WizardLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/protocols/CloakLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/protocols/OpenVpnLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/protocols/OtherProtocolsLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/protocols/PageProtocolLogicBase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/pages_logic/protocols/ShadowSocksLogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/servers_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/uilogic.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/qautostart.cpp
    ${CMAKE_CURRENT_LIST_DIR}/ui/models/sites_model.cpp
    ${CMAKE_CURRENT_LIST_DIR}/utilities.cpp
    ${CMAKE_CURRENT_LIST_DIR}/vpnconnection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/protocols/vpnprotocol.cpp
    ${CMAKE_CURRENT_LIST_DIR}/logger.cpp
    ${CMAKE_CURRENT_LIST_DIR}/loghandler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/platforms/ios/QRCodeReaderBase.cpp
)

qt6_add_resources(QRC ${QRC} ${CMAKE_CURRENT_LIST_DIR}/resources.qrc)

#todo add translations

if(WIN32)
    add_compile_definitions(MVPN_WINDOWS)

    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/protocols/ikev2_vpn_protocol_windows.h
        ${CMAKE_CURRENT_LIST_DIR}/ui/framelesswindow.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/protocols/ikev2_vpn_protocol_windows.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ui/framelesswindow.cpp
    )

    set(RESOURCES ${RESOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/platform_win/vpnclient.rc
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

    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        message("Windows x86_64 build")
        link_directories(${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/lib/windows/x86_64)
        set(LIBS ${LIBS}
            libssl
            libcrypto
        )
    else()
        message("Windows x86 build")
        link_directories(${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/lib/windows/x86)
        set(LIBS ${LIBS}
            libssl
            libcrypto
        )
    endif()
endif()
    
if(APPLE)
    add_compile_definitions(MVPN_MACOS)

    # ICON   = $$PWD/images/app.icns //todo

    set(HEADERS ${HEADERS} ${CMAKE_CURRENT_LIST_DIR}/ui/macos_util.h)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/ui/macos_util.mm)

    # LIBS += -framework Cocoa -framework ApplicationServices -framework CoreServices -framework Foundation -framework AppKit -framework Security

    # LIBS += $$PWD/3rd/OpenSSL/lib/macos/x86_64/libcrypto.a
    # LIBS += $$PWD/3rd/OpenSSL/lib/macos/x86_64/libssl.a

endif()

if(LINUX AND NOT ANDROID)
    add_compile_definitions(MVPN_LINUX)

    set(LIBS ${LIBS}
        /usr/lib/x86_64-linux-gnu/libcrypto.a
        /usr/lib/x86_64-linux-gnu/libssl.a
    )
    link_directories(${CMAKE_CURRENT_LIST_DIR}/platforms/linux)
endif()

if(WIN32 OR APPLE OR (LINUX AND NOT ANDROID))
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
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/core/ipcclient.cpp
        ${CMAKE_CURRENT_LIST_DIR}/core/privileged_process.cpp
        ${CMAKE_CURRENT_LIST_DIR}/ui/systemtray_notificationhandler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/openvpnprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/openvpnovercloakprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/shadowsocksvpnprotocol.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/wireguardprotocol.cpp
    )
endif()

if(ANDROID)
    message("Client android ${CMAKE_ANDROID_ARCH_ABI} build")
    # We need to include qtprivate api's
    # As QAndroidBinder is not yet implemented with a public api
    set(LIBS ${LIBS} Qt6::CorePrivate)
    set(ANDROID_ABIS ANDROID_TARGET_ARCH)

    add_compile_definitions(MVPN_ANDROID)

    link_directories(${CMAKE_CURRENT_LIST_DIR}/platforms/android)

    set(HEADERS ${HEADERS}
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/android_controller.h
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/android_notificationhandler.h
        ${CMAKE_CURRENT_LIST_DIR}/protocols/android_vpnprotocol.h
    )

    set(SOURCES ${SOURCES}
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/android_controller.cp
        ${CMAKE_CURRENT_LIST_DIR}/platforms/android/android_notificationhandler.cpp
        ${CMAKE_CURRENT_LIST_DIR}/protocols/android_vpnprotocol.cpp
    )

    # DISTFILES += 
    #     android/AndroidManifest.xml 
    #     android/build.gradle 
    #     android/gradle/wrapper/gradle-wrapper.jar 
    #     android/gradle/wrapper/gradle-wrapper.properties 
    #     android/gradlew 
    #     android/gradlew.bat 
    #     android/gradle.properties 
    #     android/res/values/libs.xml 
    #     android/src/org/amnezia/vpn/OpenVPNThreadv3.kt 
    #     android/src/org/amnezia/vpn/VpnService.kt 
    #     android/src/org/amnezia/vpn/VpnServiceBinder.kt 
    #     android/src/org/amnezia/vpn/qt/VPNPermissionHelper.kt

    set(QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/android)

    foreach(abi IN ANDROID_ABIS)
        if(ANDROID_TARGET_ARCH EQUAL ${abi})
            set(LIBS ${LIBS} 
                ${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/lib/android/${abi}/libcrypto.a
                ${CMAKE_CURRENT_LIST_DIR}/3rd/OpenSSL/lib/android/${abi}/libssl.a
            )
        endif()

        set(QT_ANDROID_EXTRA_LIBS ${QT_ANDROID_EXTRA_LIBS}
            ${QT_ANDROID_EXTRA_LIBS}/android/lib/wireguard/${abi}/libwg.so
            ${QT_ANDROID_EXTRA_LIBS}/android/lib/wireguard/${abi}/libwg-go.so
            ${QT_ANDROID_EXTRA_LIBS}/android/lib/wireguard/${abi}/libwg-quick.so
    
            ${QT_ANDROID_EXTRA_LIBS}/android/lib/openvpn/${abi}/libjbcrypto.so
            ${QT_ANDROID_EXTRA_LIBS}/android/lib/openvpn/${abi}/libopenvpn.so
            ${QT_ANDROID_EXTRA_LIBS}/android/lib/openvpn/${abi}/libopvpnutil.so
            ${QT_ANDROID_EXTRA_LIBS}/android/lib/openvpn/${abi}/libovpn3.so
            ${QT_ANDROID_EXTRA_LIBS}/android/lib/openvpn/${abi}/libovpnexec.so
        )
    endforeach()
endif()

if(IOS)
    message("Client iOS build")

    # CONFIG += static
    # CONFIG += file_copies

    # # For the authentication
    # LIBS += -framework AuthenticationServices

    # # For notifications
    # LIBS += -framework UIKit
    # LIBS += -framework Foundation
    # LIBS += -framework StoreKit
    # LIBS += -framework UserNotifications

    add_compile_definitions(MVPN_IOS)

    set(HEADERS ${HEADERS}
        ${QT_ANDROID_EXTRA_LIBS}/protocols/ios_vpnprotocol.h 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/iosnotificationhandler.h 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/json.h 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/bigint.h 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/bigintipv6addr.h 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/ipaddress.h 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/ipaddressrange.h 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/QtAppDelegate.h 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/QtAppDelegate-C-Interface.h
    )

    # i think in cmake it is not necessary?
    # SOURCES -= 
    #   platforms/ios/QRCodeReaderBase.cpp 
    #   platforms/ios/MobileUtils.cpp

    set(SOURCES ${SOURCES}
        ${QT_ANDROID_EXTRA_LIBS}/protocols/ios_vpnprotocol.mm 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/iosnotificationhandler.mm 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/json.cpp 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/iosglue.mm 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/ipaddress.cpp 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/ipaddressrange.cpp 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/QRCodeReaderBase.mm 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/QtAppDelegate.mm 
        ${QT_ANDROID_EXTRA_LIBS}/platforms/ios/MobileUtils.mm
    )

    # Q_ENABLE_BITCODE.value = NO
    # Q_ENABLE_BITCODE.name = ENABLE_BITCODE
    # QMAKE_MAC_XCODE_SETTINGS += Q_ENABLE_BITCODE
endif()

#if(IPHONEOS)
#endif()
#todo add iphones

qt_add_executable(${PROJECT} ${SOURCES} ${HEADERS} ${RESOURCES} ${QRC})
target_link_libraries(${PROJECT} PRIVATE 
    Qt6::Widgets Qt6::Core Qt6::Gui
    Qt6::Network Qt6::Xml Qt6::RemoteObjects
    Qt6::Quick Qt6::Svg Qt6::QuickControls2
    Qt6::Core5Compat ${LIBS}
)

if(WIN32) #todo add mac and linux
    qt_add_repc_replicas(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc_interface.rep)
    qt_add_repc_replicas(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/../ipc/ipc_process_interface.rep)
endif()
