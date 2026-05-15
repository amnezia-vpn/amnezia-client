#ifndef SCRIPTSREGISTRY_H
#define SCRIPTSREGISTRY_H

#include <QLatin1String>
#include <QList>
#include <QPair>
#include <QString>
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/utils/containerEnum.h"
#include "core/utils/containers/containerUtils.h"
#include "core/utils/protocolEnum.h"
#include "core/models/containerConfig.h"

namespace amnezia {

typedef QList<QPair<QString, QString>> ScriptVars;

enum SharedScriptType {
    // General scripts
    prepare_host,
    install_docker,
    build_container,
    remove_container,
    remove_all_containers,
    setup_host_firewall,
    check_connection,
    check_server_is_busy,
    check_user_in_sudo
};
enum ProtocolScriptType {
    // Protocol scripts
    dockerfile,
    run_container,
    configure_container,
    container_startup,
    openvpn_template,
    wireguard_template,
    awg_template,
    xray_template
};

enum ClientScriptType {
    // Client-side scripts
    mac_installer
};

QString scriptFolder(DockerContainer container);

QString scriptName(SharedScriptType type);
QString scriptName(ProtocolScriptType type);
QString scriptName(ClientScriptType type);

QString scriptData(SharedScriptType type);
QString scriptData(ProtocolScriptType type, DockerContainer container);
QString scriptData(ClientScriptType type);

ScriptVars genBaseVars(const ServerCredentials &credentials, 
                       DockerContainer container,
                       const QString &primaryDns,
                       const QString &secondaryDns);

ScriptVars genOpenVpnVars(const ContainerConfig &containerConfig);
ScriptVars genXrayVars(const ContainerConfig &containerConfig);
ScriptVars genWireGuardVars(const ContainerConfig &containerConfig);
ScriptVars genAwgVars(const ContainerConfig &containerConfig);
ScriptVars genSftpVars(const ContainerConfig &containerConfig);
ScriptVars genSocks5ProxyVars(const ContainerConfig &containerConfig);

ScriptVars genProtocolVarsForContainer(DockerContainer container, const ContainerConfig &containerConfig);
}

#endif // SCRIPTSREGISTRY_H
