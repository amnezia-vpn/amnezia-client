set(CLIENT_ROOT_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

set(HEADERS ${HEADERS}
    ${CLIENT_ROOT_DIR}/migrations.h
    ${CLIENT_ROOT_DIR}/../ipc/ipc.h
    ${CLIENT_ROOT_DIR}/amnezia_application.h
    ${CLIENT_ROOT_DIR}/core/models/containers/containers_defs.h
    ${CLIENT_ROOT_DIR}/core/defs.h
    ${CLIENT_ROOT_DIR}/core/errorstrings.h
    ${CLIENT_ROOT_DIR}/core/scripts_registry.h
    ${CLIENT_ROOT_DIR}/core/server_defs.h
    ${CLIENT_ROOT_DIR}/core/api/apiDefs.h
    ${CLIENT_ROOT_DIR}/core/qrCodeUtils.h
    ${CLIENT_ROOT_DIR}/core/controllers/coreController.h
    ${CLIENT_ROOT_DIR}/core/controllers/api/gatewayController.h
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/serverController.h
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/selfhostedConfigController.h
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/installationController.h
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/clientManagementController.h
    ${CLIENT_ROOT_DIR}/core/controllers/configController.h
    ${CLIENT_ROOT_DIR}/core/controllers/dnsController.h
    ${CLIENT_ROOT_DIR}/core/controllers/splitTunnelingController.h
    ${CLIENT_ROOT_DIR}/core/controllers/settingsController.h
    ${CLIENT_ROOT_DIR}/core/controllers/vpnConnectionController.h
    ${CLIENT_ROOT_DIR}/core/controllers/vpnConfigurationController.h
    ${CLIENT_ROOT_DIR}/protocols/protocols_defs.h
    ${CLIENT_ROOT_DIR}/protocols/qml_register_protocols.h
    ${CLIENT_ROOT_DIR}/ui/pages.h
    ${CLIENT_ROOT_DIR}/ui/qautostart.h
    ${CLIENT_ROOT_DIR}/protocols/vpnprotocol.h
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
    ${CLIENT_ROOT_DIR}/core/sshclient.h
    ${CLIENT_ROOT_DIR}/core/networkUtilities.h
    ${CLIENT_ROOT_DIR}/core/serialization/serialization.h
    ${CLIENT_ROOT_DIR}/core/serialization/transfer.h
    ${CLIENT_ROOT_DIR}/../common/logger/logger.h
    ${CLIENT_ROOT_DIR}/utils/qmlUtils.h
    ${CLIENT_ROOT_DIR}/core/api/apiUtils.h
)

# Mozilla headres
set(HEADERS ${HEADERS}
    ${CLIENT_ROOT_DIR}/mozilla/models/server.h
    ${CLIENT_ROOT_DIR}/mozilla/shared/ipaddress.h
    ${CLIENT_ROOT_DIR}/mozilla/shared/leakdetector.h
    ${CLIENT_ROOT_DIR}/mozilla/controllerimpl.h
    ${CLIENT_ROOT_DIR}/mozilla/localsocketcontroller.h
)

if(NOT IOS)
    set(HEADERS ${HEADERS}
        ${CLIENT_ROOT_DIR}/platforms/ios/QRCodeReaderBase.h
    )
endif()

if(NOT ANDROID)
    set(HEADERS ${HEADERS}
        ${CLIENT_ROOT_DIR}/ui/notificationhandler.h
    )
endif()

set(SOURCES ${SOURCES}
    ${CLIENT_ROOT_DIR}/migrations.cpp
    ${CLIENT_ROOT_DIR}/amnezia_application.cpp
    ${CLIENT_ROOT_DIR}/core/models/containers/containers_defs.cpp
    ${CLIENT_ROOT_DIR}/core/errorstrings.cpp
    ${CLIENT_ROOT_DIR}/core/scripts_registry.cpp
    ${CLIENT_ROOT_DIR}/core/server_defs.cpp
    ${CLIENT_ROOT_DIR}/core/qrCodeUtils.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/coreController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/api/gatewayController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/serverController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/selfhostedConfigController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/installationController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/selfhosted/clientManagementController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/configController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/dnsController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/splitTunnelingController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/settingsController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/vpnConnectionController.cpp
    ${CLIENT_ROOT_DIR}/core/controllers/vpnConfigurationController.cpp
    ${CLIENT_ROOT_DIR}/protocols/protocols_defs.cpp
    ${CLIENT_ROOT_DIR}/ui/qautostart.cpp
    ${CLIENT_ROOT_DIR}/protocols/vpnprotocol.cpp
    ${CLIENT_ROOT_DIR}/core/sshclient.cpp
    ${CLIENT_ROOT_DIR}/core/networkUtilities.cpp
    ${CLIENT_ROOT_DIR}/core/serialization/outbound.cpp
    ${CLIENT_ROOT_DIR}/core/serialization/inbound.cpp
    ${CLIENT_ROOT_DIR}/core/serialization/ss.cpp
    ${CLIENT_ROOT_DIR}/core/serialization/ssd.cpp
    ${CLIENT_ROOT_DIR}/core/serialization/vless.cpp
    ${CLIENT_ROOT_DIR}/core/serialization/trojan.cpp
    ${CLIENT_ROOT_DIR}/core/serialization/vmess.cpp
    ${CLIENT_ROOT_DIR}/core/serialization/vmess_new.cpp
    ${CLIENT_ROOT_DIR}/../common/logger/logger.cpp
    ${CLIENT_ROOT_DIR}/utils/qmlUtils.cpp
    ${CLIENT_ROOT_DIR}/core/api/apiUtils.cpp
)

# Mozilla sources
set(SOURCES ${SOURCES}
    ${CLIENT_ROOT_DIR}/mozilla/models/server.cpp
    ${CLIENT_ROOT_DIR}/mozilla/shared/ipaddress.cpp
    ${CLIENT_ROOT_DIR}/mozilla/shared/leakdetector.cpp
    ${CLIENT_ROOT_DIR}/mozilla/localsocketcontroller.cpp
)

if(NOT IOS)
    set(SOURCES ${SOURCES}
        ${CLIENT_ROOT_DIR}/platforms/ios/QRCodeReaderBase.cpp
    )
endif()

if(NOT ANDROID)
    set(SOURCES ${SOURCES}
        ${CLIENT_ROOT_DIR}/ui/notificationhandler.cpp
    )
endif()

file(GLOB COMMON_FILES_H CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/*.h)
file(GLOB COMMON_FILES_CPP CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/*.cpp)

file(GLOB_RECURSE PAGE_LOGIC_H CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/ui/pages_logic/*.h)
file(GLOB_RECURSE PAGE_LOGIC_CPP CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/ui/pages_logic/*.cpp)

file(GLOB CONFIGURATORS_H CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/configurators/*.h)
file(GLOB CONFIGURATORS_CPP CONFIGURE_DEPENDS ${CLIENT_ROOT_DIR}/configurators/*.cpp)

file(GLOB UI_MODELS_H CONFIGURE_DEPENDS
    ${CLIENT_ROOT_DIR}/ui/models/*.h
    ${CLIENT_ROOT_DIR}/ui/models/protocols/*.h
    ${CLIENT_ROOT_DIR}/ui/models/services/*.h
    ${CLIENT_ROOT_DIR}/ui/models/api/*.h
    ${CLIENT_ROOT_DIR}/ui/models/selfhosted/*.h
)

file(GLOB UI_MODELS_CPP CONFIGURE_DEPENDS
    ${CLIENT_ROOT_DIR}/ui/models/*.cpp
    ${CLIENT_ROOT_DIR}/ui/models/protocols/*.cpp
    ${CLIENT_ROOT_DIR}/ui/models/services/*.cpp
    ${CLIENT_ROOT_DIR}/ui/models/api/*.cpp
    ${CLIENT_ROOT_DIR}/ui/models/selfhosted/*.cpp
)

file(GLOB UI_CONTROLLERS_H CONFIGURE_DEPENDS
    ${CLIENT_ROOT_DIR}/ui/controllers/*.h
    ${CLIENT_ROOT_DIR}/ui/controllers/api/*.h
    ${CLIENT_ROOT_DIR}/ui/controllers/selfhosted/*.h
)

file(GLOB UI_CONTROLLERS_CPP CONFIGURE_DEPENDS
    ${CLIENT_ROOT_DIR}/ui/controllers/*.cpp
    ${CLIENT_ROOT_DIR}/ui/controllers/api/*.cpp
    ${CLIENT_ROOT_DIR}/ui/controllers/selfhosted/*.cpp
)

file(GLOB CORE_MODELS_H CONFIGURE_DEPENDS
    ${CLIENT_ROOT_DIR}/core/models/*.h
    ${CLIENT_ROOT_DIR}/core/models/containers/*.h
    ${CLIENT_ROOT_DIR}/core/models/protocols/*.h
    ${CLIENT_ROOT_DIR}/core/models/servers/*.h
)

file(GLOB CORE_MODELS_CPP CONFIGURE_DEPENDS
    ${CLIENT_ROOT_DIR}/core/models/*.cpp
    ${CLIENT_ROOT_DIR}/core/models/containers/*.cpp
    ${CLIENT_ROOT_DIR}/core/models/protocols/*.cpp
    ${CLIENT_ROOT_DIR}/core/models/servers/*.cpp
)

set(HEADERS ${HEADERS}
    ${COMMON_FILES_H}
    ${PAGE_LOGIC_H}
    ${CONFIGURATORS_H}
    ${UI_MODELS_H}
    ${UI_CONTROLLERS_H}
    ${CORE_MODELS_H}
)
set(SOURCES ${SOURCES}
    ${COMMON_FILES_CPP}
    ${PAGE_LOGIC_CPP}
    ${CONFIGURATORS_CPP}
    ${UI_MODELS_CPP}
    ${UI_CONTROLLERS_CPP}
    ${CORE_MODELS_CPP}
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

if(WIN32 OR (APPLE AND NOT IOS) OR (LINUX AND NOT ANDROID))
    message("Client desktop build")
    add_compile_definitions(AMNEZIA_DESKTOP)

    set(HEADERS ${HEADERS}
        ${CLIENT_ROOT_DIR}/core/ipcclient.h
        ${CLIENT_ROOT_DIR}/core/privileged_process.h
        ${CLIENT_ROOT_DIR}/ui/systemtray_notificationhandler.h
        ${CLIENT_ROOT_DIR}/protocols/openvpnprotocol.h
        ${CLIENT_ROOT_DIR}/protocols/openvpnovercloakprotocol.h
        ${CLIENT_ROOT_DIR}/protocols/shadowsocksvpnprotocol.h
        ${CLIENT_ROOT_DIR}/protocols/wireguardprotocol.h
        ${CLIENT_ROOT_DIR}/protocols/xrayprotocol.h
        ${CLIENT_ROOT_DIR}/protocols/awgprotocol.h
    )

    set(SOURCES ${SOURCES}
        ${CLIENT_ROOT_DIR}/core/ipcclient.cpp
        ${CLIENT_ROOT_DIR}/core/privileged_process.cpp
        ${CLIENT_ROOT_DIR}/ui/systemtray_notificationhandler.cpp
        ${CLIENT_ROOT_DIR}/protocols/openvpnprotocol.cpp
        ${CLIENT_ROOT_DIR}/protocols/openvpnovercloakprotocol.cpp
        ${CLIENT_ROOT_DIR}/protocols/shadowsocksvpnprotocol.cpp
        ${CLIENT_ROOT_DIR}/protocols/wireguardprotocol.cpp
        ${CLIENT_ROOT_DIR}/protocols/xrayprotocol.cpp
        ${CLIENT_ROOT_DIR}/protocols/awgprotocol.cpp
    )
endif()
