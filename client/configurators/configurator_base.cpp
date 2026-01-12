#include "configurator_base.h"

#include "configurators/awg_configurator.h"
#include "configurators/cloak_configurator.h"
#include "configurators/ikev2_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/wireguard_configurator.h"
#include "configurators/xray_configurator.h"

ConfiguratorBase::ConfiguratorBase(std::shared_ptr<Settings> settings, SshSession* sshSession, QObject *parent)
    : QObject { parent }, m_settings(settings), m_sshSession(sshSession)
{
}

QScopedPointer<ConfiguratorBase> ConfiguratorBase::create(Proto protocol,
                                                          std::shared_ptr<Settings> settings,
                                                          SshSession* sshSession)
{
    switch (protocol) {
    case Proto::OpenVpn: return QScopedPointer<ConfiguratorBase>(new OpenVpnConfigurator(settings, sshSession));
    case Proto::ShadowSocks: return QScopedPointer<ConfiguratorBase>(new ShadowSocksConfigurator(settings, sshSession));
    case Proto::Cloak: return QScopedPointer<ConfiguratorBase>(new CloakConfigurator(settings, sshSession));
    case Proto::WireGuard: return QScopedPointer<ConfiguratorBase>(new WireguardConfigurator(settings, sshSession, false));
    case Proto::Awg: return QScopedPointer<ConfiguratorBase>(new AwgConfigurator(settings, sshSession));
    case Proto::Ikev2: return QScopedPointer<ConfiguratorBase>(new Ikev2Configurator(settings, sshSession));
    case Proto::Xray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator(settings, sshSession));
    case Proto::SSXray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator(settings, sshSession));
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
