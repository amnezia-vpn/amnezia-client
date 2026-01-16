#include "configuratorBase.h"

#include "core/configurators/awgConfigurator.h"
#include "core/configurators/ikev2Configurator.h"
#include "core/configurators/openVpnConfigurator.h"
#include "core/configurators/wireguardConfigurator.h"
#include "core/configurators/xrayConfigurator.h"

ConfiguratorBase::ConfiguratorBase(AppSettingsRepository* appSettingsRepository, SshSession* sshSession, QObject *parent)
    : QObject { parent }, m_appSettingsRepository(appSettingsRepository), m_sshSession(sshSession)
{
}

QScopedPointer<ConfiguratorBase> ConfiguratorBase::create(Proto protocol,
                                                          AppSettingsRepository* appSettingsRepository,
                                                          SshSession* sshSession)
{
    switch (protocol) {
    case Proto::OpenVpn: return QScopedPointer<ConfiguratorBase>(new OpenVpnConfigurator(appSettingsRepository, sshSession));
    case Proto::WireGuard: return QScopedPointer<ConfiguratorBase>(new WireguardConfigurator(appSettingsRepository, sshSession, false));
    case Proto::Awg: return QScopedPointer<ConfiguratorBase>(new AwgConfigurator(appSettingsRepository, sshSession));
    case Proto::Ikev2: return QScopedPointer<ConfiguratorBase>(new Ikev2Configurator(appSettingsRepository, sshSession));
    case Proto::Xray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator(appSettingsRepository, sshSession));
    case Proto::SSXray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator(appSettingsRepository, sshSession));
    default: return QScopedPointer<ConfiguratorBase>();
    }
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
