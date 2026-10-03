#include "configuratorBase.h"

#include <QStringList>

#include "core/configurators/awgConfigurator.h"
#include "core/configurators/ikev2Configurator.h"
#include "core/configurators/openVpnConfigurator.h"
#include "core/configurators/wireguardConfigurator.h"
#include "core/configurators/xrayConfigurator.h"

using namespace amnezia;

namespace {

QString stripEmptyDnsEntries(const QString &config)
{
    const QString dhcpOption = QStringLiteral("dhcp-option DNS");
    QStringList filtered;
    for (const QString &line : config.split('\n')) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(dhcpOption)
            && trimmed.mid(dhcpOption.size()).trimmed().isEmpty()) {
            continue;
        }

        const int separator = trimmed.indexOf('=');
        if (separator > 0
            && trimmed.left(separator).trimmed().compare(QStringLiteral("DNS"), Qt::CaseInsensitive) == 0) {
            QStringList servers;
            for (const QString &value : trimmed.mid(separator + 1).split(',', Qt::SkipEmptyParts)) {
                const QString server = value.trimmed();
                if (!server.isEmpty()) {
                    servers.append(server);
                }
            }
            if (servers.isEmpty()) {
                continue;
            }
            filtered.append(QStringLiteral("DNS = ") + servers.join(QStringLiteral(", ")));
            continue;
        }

        filtered.append(line);
    }
    return filtered.join('\n');
}

} // namespace

ConfiguratorBase::ConfiguratorBase(SshSession* sshSession, QObject *parent)
    : QObject { parent }, m_sshSession(sshSession)
{
}

QScopedPointer<ConfiguratorBase> ConfiguratorBase::create(Proto protocol,
                                                          SshSession* sshSession)
{
    switch (protocol) {
    case Proto::OpenVpn: return QScopedPointer<ConfiguratorBase>(new OpenVpnConfigurator(sshSession));
    case Proto::WireGuard: return QScopedPointer<ConfiguratorBase>(new WireguardConfigurator(sshSession, false));
    case Proto::Awg: return QScopedPointer<ConfiguratorBase>(new AwgConfigurator(sshSession));
    case Proto::Ikev2: return QScopedPointer<ConfiguratorBase>(new Ikev2Configurator(sshSession));
    case Proto::Xray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator(sshSession));
    case Proto::SSXray: return QScopedPointer<ConfiguratorBase>(new XrayConfigurator(sshSession));
    default: return QScopedPointer<ConfiguratorBase>();
    }
}

ProtocolConfig ConfiguratorBase::processConfigWithLocalSettings(const ConnectionSettings &settings,
                                                                 ProtocolConfig protocolConfig)
{
    applyDnsToNativeConfig(settings.dns, protocolConfig);
    return protocolConfig;
}

ProtocolConfig ConfiguratorBase::processConfigWithExportSettings(const ExportSettings &settings,
                                                                 ProtocolConfig protocolConfig)
{
    applyDnsToNativeConfig(settings.dns, protocolConfig);
    return protocolConfig;
}

void ConfiguratorBase::applyDnsToNativeConfig(const DnsSettings &dns, ProtocolConfig &protocolConfig)
{
    QString config = protocolConfig.nativeConfig();
    config.replace("$PRIMARY_DNS", dns.primaryDns);
    config.replace("$SECONDARY_DNS", dns.secondaryDns);
    protocolConfig.setNativeConfig(stripEmptyDnsEntries(config));
}
