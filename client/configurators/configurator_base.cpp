#include "configurator_base.h"

ConfiguratorBase::ConfiguratorBase(std::shared_ptr<Settings> settings, QObject *parent)
    : QObject { parent }, m_settings(settings)
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

//void ConfiguratorBase::updateContainerConfigAfterInstallation(const DockerContainer container,
//                                                              QJsonObject &containerConfig, const QString &stdOut)
//{
//    Proto mainProto = ContainerProps::defaultProtocol(container);

//    if (container == DockerContainer::TorWebSite) {
//        QJsonObject protocol = containerConfig.value(ProtocolProps::protoToString(mainProto)).toObject();

//        qDebug() << "amnezia-tor onions" << stdOut;

//        QString onion = stdOut;
//        onion.replace("\n", "");
//        protocol.insert(config_key::site, onion);

//        containerConfig.insert(ProtocolProps::protoToString(mainProto), protocol);
//    }
//}
