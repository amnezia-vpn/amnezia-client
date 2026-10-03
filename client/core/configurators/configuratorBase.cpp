#include "configuratorBase.h"

#include <QStringList>

#include "core/utils/networkUtilities.h"
#include "core/configurators/awgConfigurator.h"
#include "core/configurators/ikev2Configurator.h"
#include "core/configurators/openVpnConfigurator.h"
#include "core/configurators/wireguardConfigurator.h"
#include "core/configurators/xrayConfigurator.h"

using namespace amnezia;

namespace {

QString stripEmptyOpenVpnDnsEntries(const QString &config)
{
    const QString dhcpOption = QStringLiteral("dhcp-option DNS");
    QStringList filtered;
    for (const QString &line : config.split('\n')) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(dhcpOption)
            && trimmed.mid(dhcpOption.size()).trimmed().isEmpty()) {
            continue;
        }
        filtered.append(line);
    }
    return filtered.join('\n');
}

QString stripEmptyWireguardDnsEntries(const QString &config)
{
    QStringList filtered;
    for (const QString &line : config.split('\n')) {
        const QString trimmed = line.trimmed();
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
            // Keep the original indentation; only the value list is rebuilt.
            QString rebuilt = line.left(line.indexOf('=') + 1) + QLatin1Char(' ') + servers.join(QStringLiteral(", "));
            if (line.endsWith(QLatin1Char('\r'))) {
                rebuilt.append(QLatin1Char('\r'));
            }
            filtered.append(rebuilt);
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

void ConfiguratorBase::applyDnsToNativeConfig(const DnsSettings &dns, ProtocolConfig &protocolConfig,
                                              DnsEntryCleanup cleanup)
{
    // The values can come from an imported settings backup: do not let anything that is
    // not an IPv4 address be textually injected into a native config, where it is on a
    // line of its own and would be interpreted as further directives (#3251 review).
    const auto sanitizeDns = [](const QString &address) {
        return address.isEmpty() || NetworkUtilities::checkIPv4Format(address) ? address : QString();
    };

    QString config = protocolConfig.nativeConfig();
    config.replace("$PRIMARY_DNS", sanitizeDns(dns.primaryDns));
    config.replace("$SECONDARY_DNS", sanitizeDns(dns.secondaryDns));
    switch (cleanup) {
    case DnsEntryCleanup::OpenVpn:
        config = stripEmptyOpenVpnDnsEntries(config);
        break;
    case DnsEntryCleanup::Wireguard:
        config = stripEmptyWireguardDnsEntries(config);
        break;
    case DnsEntryCleanup::None:
        break;
    }
    protocolConfig.setNativeConfig(config);
}
