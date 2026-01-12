#include "configurator_base.h"

#include "configurators/awg_configurator.h"
#include "configurators/cloak_configurator.h"
#include "configurators/ikev2_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/wireguard_configurator.h"
#include "configurators/xray_configurator.h"

ConfiguratorBase::ConfiguratorBase(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent)
    : QObject { parent }, m_settings(settings), m_serverController(serverController)
{
}

QScopedPointer<ConfiguratorBase> ConfiguratorBase::create(Proto protocol,
                                                          std::shared_ptr<Settings> settings,
                                                          const QSharedPointer<ServerController> &serverController)
{
    switch (protocol) {
    case Proto::OpenVpn: return QScopedPointer<ConfiguratorBase>(new OpenVpnConfigurator(settings, serverController));
    case Proto::ShadowSocks: return QScopedPointer<ConfiguratorBase>(new ShadowSocksConfigurator(settings, serverController));
    case Proto::Cloak: return QScopedPointer<ConfiguratorBase>(new CloakConfigurator(settings, serverController));
    case Proto::WireGuard: return QScopedPointer<ConfiguratorBase>(new WireguardConfigurator(settings, serverController, false));
    case Proto::Awg: return QScopedPointer<ConfiguratorBase>(new AwgConfigurator(settings, serverController));
    case Proto::Ikev2: return QScopedPointer<ConfiguratorBase>(new Ikev2Configurator(settings, serverController));
    case Proto::Xray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator(settings, serverController));
    case Proto::SSXray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator(settings, serverController));
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
