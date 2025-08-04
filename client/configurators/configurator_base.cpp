#include "configurator_base.h"
#include "core/networkUtilities.h"

ConfiguratorBase::ConfiguratorBase(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : QObject { parent }, m_settings(settings), m_serverController(serverController)
{
}

QString ConfiguratorBase::processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                         QString &protocolConfigString)
{
    processConfigWithDnsSettings(dns, protocolConfigString);
    return protocolConfigString;
}

QString ConfiguratorBase::processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                          QString &protocolConfigString)
{
    processConfigWithDnsSettings(dns, protocolConfigString);
    return protocolConfigString;
}

void ConfiguratorBase::processConfigWithDnsSettings(const QPair<QString, QString> &dns, QString &protocolConfigString)
{
    protocolConfigString.replace("$PRIMARY_DNS", dns.first);
    protocolConfigString.replace("$SECONDARY_DNS", dns.second);
}

ConfiguratorBase::Vars ConfiguratorBase::generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                                                             const QSharedPointer<ProtocolConfig> &protocolConfig) const
{
    return generateCommonVars(credentials, container);
}

ConfiguratorBase::Vars ConfiguratorBase::generateCommonVars(const ServerCredentials &credentials, DockerContainer container) const
{
    Vars vars;

    vars.append({{"$REMOTE_HOST", credentials.hostName}});

    QString serverIp = (container != DockerContainer::Awg && container != DockerContainer::WireGuard && container != DockerContainer::Xray)
            ? NetworkUtilities::getIPAddress(credentials.hostName)
            : credentials.hostName;
    if (!serverIp.isEmpty()) {
        vars.append({{"$SERVER_IP_ADDRESS", serverIp}});
    }

    vars.append({{"$CONTAINER_NAME", ContainerProps::containerToString(container)}});
    vars.append({{"$DOCKERFILE_FOLDER", "/opt/amnezia/" + ContainerProps::containerToString(container)}});

    vars.append({{"$PRIMARY_SERVER_DNS", m_settings->primaryDns()}});
    vars.append({{"$SECONDARY_SERVER_DNS", m_settings->secondaryDns()}});

    return vars;
}
