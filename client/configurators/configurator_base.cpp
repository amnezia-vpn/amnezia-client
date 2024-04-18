#include "configurator_base.h"

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
