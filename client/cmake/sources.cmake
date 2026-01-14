set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

set(HEADERS ${HEADERS}
    ${CLIENT_ROOT_DIR}/core/utils/migrations.h
    ${CLIENT_ROOT_DIR}/../ipc/ipc.h
    ${CLIENT_ROOT_DIR}/amnezia_application.h
    ${CLIENT_ROOT_DIR}/containers/containers_defs.h
    ${CLIENT_ROOT_DIR}/core/utils/defs.h
    ${CLIENT_ROOT_DIR}/core/utils/errorStrings.h
    ${CLIENT_ROOT_DIR}/core/utils/selfhosted/scriptsRegistry.h
    ${CLIENT_ROOT_DIR}/core/utils/api/apiDefs.h
    ${CLIENT_ROOT_DIR}/core/utils/qrCodeUtils.h
    ${CLIENT_ROOT_DIR}/core/controllers/coreController.h
    ${CLIENT_ROOT_DIR}/core/controllers/coreSignalHandlers.h
    ${CLIENT_ROOT_DIR}/core/controllers/gatewayController.h
    ${CLIENT_ROOT_DIR}/core/utils/selfhosted/sshSession.h
    ${CLIENT_ROOT_DIR}/core/controllers/serversController.h
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/usersController.h
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/installController.h
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/exportController.h
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/importController.h
    ${CLIENT_ROOT_DIR}/core/installers/installerBase.h
    ${CLIENT_ROOT_DIR}/core/installers/awgInstaller.h
    ${CLIENT_ROOT_DIR}/core/installers/wireguardInstaller.h
    ${CLIENT_ROOT_DIR}/core/installers/openvpnInstaller.h
    ${CLIENT_ROOT_DIR}/core/installers/xrayInstaller.h
    ${CLIENT_ROOT_DIR}/core/installers/torInstaller.h
    ${CLIENT_ROOT_DIR}/core/installers/sftpInstaller.h
    ${CLIENT_ROOT_DIR}/core/installers/socks5Installer.h
    ${CLIENT_ROOT_DIR}/core/controllers/appSplitTunnelingController.h
    ${CLIENT_ROOT_DIR}/core/controllers/sitesController.h
    ${CLIENT_ROOT_DIR}/core/controllers/allowedDnsController.h
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/exportController.h
    ${CLIENT_ROOT_DIR}/core/controllers/connectionController.h
    ${CLIENT_ROOT_DIR}/core/controllers/settingsController.h
    ${CLIENT_ROOT_DIR}/core/controllers/api/servicesCatalogController.h
    ${CLIENT_ROOT_DIR}/core/controllers/api/subscriptionController.h
    ${CLIENT_ROOT_DIR}/core/controllers/api/newsController.h
    ${CLIENT_ROOT_DIR}/core/repositories/serversRepository.h
    ${CLIENT_ROOT_DIR}/core/repositories/appSettingsRepository.h
    ${CLIENT_ROOT_DIR}/core/repositories/secureServersRepository.h
    ${CLIENT_ROOT_DIR}/core/repositories/secureAppSettingsRepository.h
    ${CLIENT_ROOT_DIR}/core/repositories/qServersRepository.h
    ${CLIENT_ROOT_DIR}/core/repositories/qAppSettingsRepository.h
    ${CLIENT_ROOT_DIR}/protocols/protocols_defs.h
    ${CLIENT_ROOT_DIR}/protocols/qml_register_protocols.h
    ${CLIENT_ROOT_DIR}/ui/utils/pages.h
    ${CLIENT_ROOT_DIR}/ui/utils/qAutoStart.h
    ${CLIENT_ROOT_DIR}/protocols/vpnprotocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    ${CLIENT_ROOT_DIR}/core/utils/selfhosted/sshClient.h
    ${CLIENT_ROOT_DIR}/core/utils/networkUtilities.h
    ${CLIENT_ROOT_DIR}/core/utils/serialization/serialization.h
    ${CLIENT_ROOT_DIR}/core/utils/serialization/transfer.h
    ${CLIENT_ROOT_DIR}/../common/logger/logger.h
    ${CLIENT_ROOT_DIR}/ui/utils/qmlUtils.h
    ${CLIENT_ROOT_DIR}/core/utils/api/apiUtils.h
    ${CLIENT_ROOT_DIR}/core/utils/osSignalHandler.h
    ${CLIENT_ROOT_DIR}/core/utils/utilities.h
    ${CLIENT_ROOT_DIR}/core/utils/managementServer.h
    ${CLIENT_ROOT_DIR}/core/utils/constants.h
)

# Mozilla headres
set(HEADERS ${HEADERS}
    ${CLIENT_ROOT_DIR}/mozilla/models/server.h
    ${CLIENT_ROOT_DIR}/mozilla/shared/ipaddress.h
    ${CLIENT_ROOT_DIR}/mozilla/shared/leakdetector.h
    ${CLIENT_ROOT_DIR}/mozilla/controllerimpl.h
)

if(NOT IOS AND NOT MACOS_NE)
    set(HEADERS ${HEADERS}
        ${CLIENT_ROOT_DIR}/platforms/ios/QRCodeReaderBase.h
    )
endif()

if(NOT ANDROID)
    set(HEADERS ${HEADERS}
        ${CLIENT_ROOT_DIR}/ui/utils/notificationHandler.h
    )
endif()

set(SOURCES ${SOURCES}
    ${CLIENT_ROOT_DIR}/core/utils/migrations.cpp
    ${CLIENT_ROOT_DIR}/amnezia_application.cpp
    ${CLIENT_ROOT_DIR}/containers/containers_defs.cpp
    ${CLIENT_ROOT_DIR}/core/utils/errorStrings.cpp
    ${CLIENT_ROOT_DIR}/core/utils/selfhosted/scriptsRegistry.cpp
    ${CLIENT_ROOT_DIR}/core/utils/qrCodeUtils.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/coreController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/coreSignalHandlers.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/gatewayController.cpp
    ${CLIENT_ROOT_DIR}/core/utils/selfhosted/sshSession.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/serversController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/usersController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/installController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/exportController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/importController.cpp
    ${CLIENT_ROOT_DIR}/core/installers/installerBase.cpp
    ${CLIENT_ROOT_DIR}/core/installers/awgInstaller.cpp
    ${CLIENT_ROOT_DIR}/core/installers/wireguardInstaller.cpp
    ${CLIENT_ROOT_DIR}/core/installers/openvpnInstaller.cpp
    ${CLIENT_ROOT_DIR}/core/installers/xrayInstaller.cpp
    ${CLIENT_ROOT_DIR}/core/installers/torInstaller.cpp
    ${CLIENT_ROOT_DIR}/core/installers/sftpInstaller.cpp
    ${CLIENT_ROOT_DIR}/core/installers/socks5Installer.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/appSplitTunnelingController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/sitesController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/allowedDnsController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/exportController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/connectionController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/settingsController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/api/servicesCatalogController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/api/subscriptionController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/api/newsController.cpp
    ${CLIENT_ROOT_DIR}/core/repositories/secureServersRepository.cpp
    ${CLIENT_ROOT_DIR}/core/repositories/secureAppSettingsRepository.cpp
    ${CLIENT_ROOT_DIR}/core/repositories/qServersRepository.cpp
    ${CLIENT_ROOT_DIR}/core/repositories/qAppSettingsRepository.cpp
    ${CLIENT_ROOT_DIR}/protocols/protocols_defs.cpp
    ${CLIENT_ROOT_DIR}/ui/utils/qAutoStart.cpp
    ${CLIENT_ROOT_DIR}/protocols/vpnprotocol.cpp
    ${CLIENT_ROOT_DIR}/core/utils/selfhosted/sshClient.cpp
    ${CLIENT_ROOT_DIR}/core/utils/networkUtilities.cpp
    ${CLIENT_ROOT_DIR}/core/utils/serialization/outbound.cpp
    ${CLIENT_ROOT_DIR}/core/utils/serialization/inbound.cpp
    ${CLIENT_ROOT_DIR}/core/utils/serialization/ss.cpp
    ${CLIENT_ROOT_DIR}/core/utils/serialization/ssd.cpp
    ${CLIENT_ROOT_DIR}/core/utils/serialization/vless.cpp
    ${CLIENT_ROOT_DIR}/core/utils/serialization/trojan.cpp
    ${CLIENT_ROOT_DIR}/core/utils/serialization/vmess.cpp
    ${CLIENT_ROOT_DIR}/core/utils/serialization/vmess_new.cpp
    ${CLIENT_ROOT_DIR}/../common/logger/logger.cpp
    ${CLIENT_ROOT_DIR}/ui/utils/qmlUtils.cpp
    ${CLIENT_ROOT_DIR}/core/utils/api/apiUtils.cpp
    ${CLIENT_ROOT_DIR}/core/utils/osSignalHandler.cpp
    ${CLIENT_ROOT_DIR}/core/utils/utilities.cpp
    ${CLIENT_ROOT_DIR}/core/utils/managementServer.cpp
)

# Mozilla sources
set(SOURCES ${SOURCES}
    ${CLIENT_ROOT_DIR}/mozilla/models/server.cpp
    ${CLIENT_ROOT_DIR}/mozilla/shared/ipaddress.cpp
    ${CLIENT_ROOT_DIR}/mozilla/shared/leakdetector.cpp
)

if(NOT IOS AND NOT MACOS_NE)
    set(SOURCES ${SOURCES}
        ${CLIENT_ROOT_DIR}/platforms/ios/QRCodeReaderBase.cpp
    )
endif()

# Include native macOS platform helpers (dock/status-item)
if(APPLE AND NOT IOS)
    list(APPEND HEADERS
        ${CLIENT_ROOT_DIR}/platforms/macos/macosutils.h
        ${CLIENT_ROOT_DIR}/platforms/macos/macosstatusicon.h
        ${CLIENT_ROOT_DIR}/ui/utils/macosUtil.h
    )
    list(APPEND SOURCES
        ${CLIENT_ROOT_DIR}/platforms/macos/macosutils.mm
        ${CLIENT_ROOT_DIR}/platforms/macos/macosstatusicon.mm
        ${CLIENT_ROOT_DIR}/ui/utils/macosUtil.mm
    )
endif()

if(NOT ANDROID)
    set(SOURCES ${SOURCES}
        ${CLIENT_ROOT_DIR}/ui/utils/notificationHandler.cpp
    )
endif()

file(GLOB COMMON_FILES_H CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/*.h)
file(GLOB COMMON_FILES_CPP CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/*.cpp)

file(GLOB_RECURSE PAGE_LOGIC_H CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/ui/pages_logic/*.h)
file(GLOB_RECURSE PAGE_LOGIC_CPP CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/ui/pages_logic/*.cpp)

file(GLOB CONFIGURATORS_H CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/core/configurators/*.h)
file(GLOB CONFIGURATORS_CPP CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/core/configurators/*.cpp)

file(GLOB UI_MODELS_H CONFIGURE_DEPENDS
    ${CLIENT_ROOT_DIR}/ui/models/*.h
    ${CLIENT_ROOT_DIR}/ui/models/protocols/*.h
    ${CLIENT_ROOT_DIR}/ui/models/services/*.h
    ${CLIENT_ROOT_DIR}/ui/models/api/*.h
)
file(GLOB UI_MODELS_CPP CONFIGURE_DEPENDS
    ${CLIENT_ROOT_DIR}/ui/models/*.cpp
    ${CLIENT_ROOT_DIR}/ui/models/protocols/*.cpp
    ${CLIENT_ROOT_DIR}/ui/models/services/*.cpp
    ${CLIENT_ROOT_DIR}/ui/models/api/*.cpp
)

file(GLOB UI_CONTROLLERS_H CONFIGURE_DEPENDS
    ${CLIENT_ROOT_DIR}/ui/controllers/*.h
    ${CLIENT_ROOT_DIR}/ui/controllers/api/*.h
    ${CLIENT_ROOT_DIR}/ui/controllers/qml/*.h
    ${CLIENT_ROOT_DIR}/ui/controllers/selfhosted/*.h
)
file(GLOB UI_CONTROLLERS_CPP CONFIGURE_DEPENDS
    ${CLIENT_ROOT_DIR}/ui/controllers/*.cpp
    ${CLIENT_ROOT_DIR}/ui/controllers/api/*.cpp
    ${CLIENT_ROOT_DIR}/ui/controllers/qml/*.cpp
    ${CLIENT_ROOT_DIR}/ui/controllers/selfhosted/*.cpp
)

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
    set(HEADERS ${HEADERS}
        ${CLIENT_ROOT_DIR}/protocols/ikev2_vpn_protocol_windows.h
    )

    set(SOURCES ${SOURCES}
        ${CLIENT_ROOT_DIR}/protocols/ikev2_vpn_protocol_windows.cpp
    )

    set(RESOURCES ${RESOURCES}
        ${CMAKE_CURRENT_BINARY_DIR}/amneziavpn.rc
    )
endif()

if(WIN32 OR (APPLE AND NOT IOS AND NOT MACOS_NE) OR (LINUX AND NOT ANDROID))
    message("Client desktop build")
    add_compile_definitions(AMNEZIA_DESKTOP)

    set(HEADERS ${HEADERS}
        ${CLIENT_ROOT_DIR}/core/utils/ipcClient.h
        ${CLIENT_ROOT_DIR}/core/utils/privilegedProcess.h
        ${CLIENT_ROOT_DIR}/ui/utils/systemTrayNotificationHandler.h
        ${CLIENT_ROOT_DIR}/protocols/openvpnprotocol.h
        ${CLIENT_ROOT_DIR}/protocols/wireguardprotocol.h
        ${CLIENT_ROOT_DIR}/protocols/xrayprotocol.h
        ${CLIENT_ROOT_DIR}/protocols/awgprotocol.h
        ${CLIENT_ROOT_DIR}/mozilla/localsocketcontroller.h
    )

    set(SOURCES ${SOURCES}
        ${CLIENT_ROOT_DIR}/core/utils/ipcClient.cpp
        ${CLIENT_ROOT_DIR}/core/utils/privilegedProcess.cpp
        ${CLIENT_ROOT_DIR}/mozilla/localsocketcontroller.cpp
        ${CLIENT_ROOT_DIR}/ui/utils/systemTrayNotificationHandler.cpp
        ${CLIENT_ROOT_DIR}/protocols/openvpnprotocol.cpp
        ${CLIENT_ROOT_DIR}/protocols/wireguardprotocol.cpp
        ${CLIENT_ROOT_DIR}/protocols/xrayprotocol.cpp
        ${CLIENT_ROOT_DIR}/protocols/awgprotocol.cpp
    )
endif()

if(APPLE AND MACOS_NE)
    # Include only the tray notification handler in NE builds
    set(HEADERS ${HEADERS}
        ${CLIENT_ROOT_DIR}/ui/utils/systemTrayNotificationHandler.h
    )

    set(SOURCES ${SOURCES}
        ${CLIENT_ROOT_DIR}/ui/utils/systemTrayNotificationHandler.cpp
    )
endif()
