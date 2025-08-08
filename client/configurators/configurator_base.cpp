#include "configurator_base.h"
#include "core/networkUtilities.h"
#include "core/models/protocols/protocolConfig.h"
#include <variant>

ConfiguratorBase::ConfiguratorBase(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : QObject { parent }, m_settings(settings), m_serverController(serverController)
{
}

void ConfiguratorBase::processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                      QSharedPointer<ProtocolConfig> &protocolConfig)
{
    processConfigWithDnsSettings(dns, protocolConfig);
}

void ConfiguratorBase::processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                       QSharedPointer<ProtocolConfig> &protocolConfig)
{
    processConfigWithDnsSettings(dns, protocolConfig);
}

void ConfiguratorBase::processConfigWithDnsSettings(const QPair<QString, QString> &dns, QSharedPointer<ProtocolConfig> &protocolConfig)
{
    ProtocolConfigVariant variant = ProtocolConfig::getProtocolConfigVariant(protocolConfig);
    std::visit([&dns](const auto &config) -> void {
        config->clientProtocolConfig.nativeConfig.replace("$PRIMARY_DNS", dns.first);
        config->clientProtocolConfig.nativeConfig.replace("$SECONDARY_DNS", dns.second);
    }, variant);
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
