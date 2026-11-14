#include <winsock2.h>

#include <iphlpapi.h>
#include <ws2tcpip.h>

#include "domainRoutingProtocol.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QTextStream>

#include <vector>

#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/ipcClient.h"
#include "core/utils/networkUtilities.h"

namespace
{
    const QString domainTunName = QStringLiteral("AmneziaDomains");

    /// <summary>Читает IPv4 DNS-серверы выбранного сетевого интерфейса.</summary>
    /// <param name="index">Индекс исходящего интерфейса.</param>
    /// <returns>Список DNS-серверов либо пустой список при ошибке.</returns>
    QStringList interfaceDns(int index)
    {
        ULONG size = 16384;
        std::vector<unsigned char> buffer(size);
        ULONG result = GetAdaptersAddresses(AF_INET, 0, nullptr,
                                            reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()), &size);
        if (result == ERROR_BUFFER_OVERFLOW) {
            buffer.resize(size);
            result = GetAdaptersAddresses(AF_INET, 0, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()),
                                          &size);
        }

        if (result != NO_ERROR) {
            return { };
        }

        QStringList servers;
        for (IP_ADAPTER_ADDRESSES *adapter = reinterpret_cast<IP_ADAPTER_ADDRESSES *>(buffer.data()); adapter;
             adapter = adapter->Next) {
            if (adapter->IfIndex != static_cast<ULONG>(index)) {
                continue;
            }

            for (IP_ADAPTER_DNS_SERVER_ADDRESS *dns = adapter->FirstDnsServerAddress; dns; dns = dns->Next) {
                if (dns->Address.lpSockaddr && dns->Address.lpSockaddr->sa_family == AF_INET) {
                    const sockaddr_in *address = reinterpret_cast<const sockaddr_in *>(dns->Address.lpSockaddr);
                    const QHostAddress ip(ntohl(address->sin_addr.s_addr));
                    if (!ip.isNull() && ip != QHostAddress::AnyIPv4) {
                        servers.append(ip.toString());
                    }
                }
            }
        }

        servers.removeDuplicates();
        return servers;
    }

    /// <summary>Читает локальные соответствия имён и адресов.</summary>
    /// <returns>Статические DNS-записи из системного файла hosts.</returns>
    QJsonObject systemHosts()
    {
        wchar_t directory[MAX_PATH];
        const UINT size = GetWindowsDirectoryW(directory, MAX_PATH);
        if (size == 0 || size >= MAX_PATH) {
            return { };
        }

        QFile file(QDir(QString::fromWCharArray(directory, size)).filePath(QStringLiteral("System32/drivers/etc/hosts")));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return { };
        }

        QJsonObject hosts;
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            const QStringList parts = stream.readLine().section('#', 0, 0).trimmed().split(
                    QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            if (parts.size() < 2 || QHostAddress(parts.first()).isNull()) {
                continue;
            }

            for (qsizetype i = 1; i < parts.size(); ++i) {
                const QString name = parts.at(i).toLower();
                QJsonArray addresses = hosts.value(name).toArray();
                if (!addresses.contains(parts.first())) {
                    addresses.append(parts.first());
                }
                hosts.insert(name, addresses);
            }
        }

        return hosts;
    }
}

DomainRoutingProtocol::DomainRoutingProtocol(const QJsonObject &configuration, QObject *parent)
    : VpnProtocol(configuration, parent)
{
    m_tunnelTimer.setInterval(100);
    m_engineTimer.setInterval(1000);
    connect(&m_tunnelTimer, &QTimer::timeout, this, &DomainRoutingProtocol::checkTunnel);
    connect(&m_engineTimer, &QTimer::timeout, this, &DomainRoutingProtocol::checkEngine);
}

DomainRoutingProtocol::~DomainRoutingProtocol()
{
    stop();
}

ErrorCode DomainRoutingProtocol::preparePaths()
{
    using namespace amnezia;
    const QString protocol = m_rawConfig.value(configKey::vpnProto).toString();
    const QJsonObject tunnel = m_rawConfig.value(protocol + QStringLiteral("_config_data")).toObject();
    const QHostAddress endpoint(NetworkUtilities::getIPAddress(tunnel.value(configKey::hostName).toString()));
    if (endpoint.protocol() != QAbstractSocket::IPv4Protocol) {
        return ErrorCode::DomainRoutingConfigInvalid;
    }

    const int index = NetworkUtilities::AdapterIndexTo(endpoint);
    const QNetworkInterface direct = QNetworkInterface::interfaceFromIndex(index);
    if (!direct.isValid() || direct.humanReadableName() == domainTunName
        || direct.humanReadableName() == QStringLiteral("AmneziaVPN")) {
        return ErrorCode::DomainRoutingConfigInvalid;
    }

    m_options.direct.interfaceName = direct.humanReadableName();
    for (const QNetworkAddressEntry &entry : direct.addressEntries()) {
        if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol && !entry.ip().isLinkLocal()) {
            m_options.direct.sourceAddress = entry.ip().toString();
            break;
        }
    }

    m_options.direct.dnsServers = interfaceDns(index);
    for (const QString &server : m_options.direct.dnsServers) {
        if (QHostAddress(server).isLoopback()) {
            return ErrorCode::DomainRoutingConfigInvalid;
        }
    }
    for (const QNetworkInterface &network : QNetworkInterface::allInterfaces()) {
        if (network.humanReadableName() == domainTunName) {
            return ErrorCode::DomainRoutingEngineBusy;
        }
    }
    m_options.tunnel.sourceAddress = tunnel.value(configKey::clientIp).toString().section('/', 0, 0);
    m_options.tunnel.interfaceName = QStringLiteral("AmneziaVPN");
    for (const QLatin1String key : { configKey::dns1, configKey::dns2 }) {
        if (key == configKey::dns2
            && m_rawConfig.value(configKey::dns1).toString().contains(protocols::dns::amneziaDnsIp)) {
            continue;
        }
        const QHostAddress dns(m_rawConfig.value(key).toString());
        if (dns.protocol() == QAbstractSocket::IPv4Protocol && !m_options.tunnel.dnsServers.contains(dns.toString())) {
            m_options.tunnel.dnsServers.append(dns.toString());
        }
    }

    m_options.mode = static_cast<RouteMode>(m_rawConfig.value(configKey::splitTunnelType).toInt());
    for (const QJsonValue &rule : m_rawConfig.value(configKey::domainRoutingRules).toArray()) {
        m_options.rules.append(rule.toString());
    }

    m_options.appsMode = static_cast<AppsRouteMode>(m_rawConfig.value(configKey::appSplitTunnelType).toInt());
    for (const QJsonValue &value : m_rawConfig.value(configKey::splitTunnelApps).toArray()) {
        const QFileInfo file(value.toString());
        if (!file.isAbsolute()) {
            return ErrorCode::DomainRoutingConfigInvalid;
        }

        const QString canonical = file.canonicalFilePath();
        m_options.apps.append(QDir::fromNativeSeparators(canonical.isEmpty() ? file.absoluteFilePath() : canonical));
    }

    m_options.hosts = systemHosts();
    m_options.tunName = domainTunName;
    bool validMtu = false;
    const int mtu = tunnel.value(configKey::mtu).toVariant().toInt(&validMtu);
    if (!validMtu || mtu < 576 || mtu > 65535) {
        return ErrorCode::DomainRoutingConfigInvalid;
    }

    m_options.mtu = static_cast<quint16>(qMax(1280, mtu));
    QString error;
    if (domainRouting::buildConfig(m_options, error).isEmpty()) {
        qWarning().noquote() << error;
        return ErrorCode::DomainRoutingConfigInvalid;
    }

    return ErrorCode::NoError;
}

ErrorCode DomainRoutingProtocol::start()
{
    resetStopGuard();
    m_options = { };
    m_tunnelConnected = false;
    const ErrorCode prepared = preparePaths();
    if (prepared != ErrorCode::NoError) {
        return prepared;
    }

    const ErrorCode availability = IpcClient::withInterface(
            [](QSharedPointer<IpcInterfaceReplica> iface) {
                auto result = iface->supportsDomainRouting();
                if (!result.waitForFinished() || !result.returnValue()) {
                    return ErrorCode::AmneziaServiceConnectionFailed;
                }
                auto token = iface->xrayCurrentToken();
                if (!token.waitForFinished()) {
                    return ErrorCode::AmneziaServiceConnectionFailed;
                }
                return token.returnValue() == 0 ? ErrorCode::NoError : ErrorCode::DomainRoutingEngineBusy;
            },
            []() { return ErrorCode::AmneziaServiceConnectionFailed; });
    if (availability != ErrorCode::NoError) {
        return availability;
    }

    QJsonObject nativeConfig = m_rawConfig;
    nativeConfig.insert(configKey::splitTunnelType, RouteMode::VpnAllSites);
    nativeConfig.insert(configKey::splitTunnelSites, QJsonArray());
    nativeConfig.insert(configKey::splitTunnelApps, QJsonArray());
    nativeConfig.insert(configKey::appSplitTunnelType, AppsRouteMode::VpnAllApps);
    m_wireguard.reset(new WireguardProtocol(nativeConfig, this));
    connect(m_wireguard.data(), &VpnProtocol::protocolError, this, &DomainRoutingProtocol::fail);
    connect(m_wireguard.data(), &VpnProtocol::bytesChanged, this, &VpnProtocol::bytesChanged);
    connect(m_wireguard.data(), &VpnProtocol::tunnelAddressesUpdated, this,
            [this](const QString &gateway, const QString &localAddress) {
                m_vpnGateway = gateway;
                m_vpnLocalAddress = localAddress;
                emit tunnelAddressesUpdated(gateway, localAddress);
            });
    connect(m_wireguard.data(), &VpnProtocol::connectionStateChanged, this, [this](Vpn::ConnectionState state) {
        if (m_stopped) {
            return;
        }

        if (state == Vpn::ConnectionState::Connected) {
            m_tunnelConnected = true;
            if (m_engineToken != 0) {
                stopTimeoutTimer();
                setConnectionState(Vpn::ConnectionState::Connected);
            }
        } else if (state == Vpn::ConnectionState::Error || state == Vpn::ConnectionState::Disconnected) {
            fail(ErrorCode::InternalError);
        }
    });

    setConnectionState(Vpn::ConnectionState::Connecting);
    const ErrorCode started = m_wireguard->start();
    if (started != ErrorCode::NoError) {
        stop();
        return started;
    }
    if (m_stopped) {
        return lastError();
    }

    startTimeoutTimer();
    m_tunnelTimer.start();
    return ErrorCode::NoError;
}

void DomainRoutingProtocol::checkTunnel()
{
    if (m_stopped) {
        return;
    }

    QNetworkInterface tunnel;
    for (const QNetworkInterface &candidate : QNetworkInterface::allInterfaces()) {
        if (candidate.humanReadableName() != m_options.tunnel.interfaceName) {
            continue;
        }

        for (const QNetworkAddressEntry &entry : candidate.addressEntries()) {
            if (entry.ip().toString() == m_options.tunnel.sourceAddress) {
                tunnel = candidate;
                break;
            }
        }
    }

    if (!tunnel.isValid()) {
        return;
    }

    m_tunnelTimer.stop();
    QString error;
    const QJsonObject config = domainRouting::buildConfig(m_options, error);
    if (config.isEmpty()) {
        fail(ErrorCode::DomainRoutingConfigInvalid);
        return;
    }
    const bool started = IpcClient::withInterface(
            [this, &config](QSharedPointer<IpcInterfaceReplica> iface) {
                auto start = iface->xrayStartExclusive(
                        QString::fromUtf8(QJsonDocument(config).toJson(QJsonDocument::Compact)));
                if (!start.waitForFinished() || start.returnValue() == 0) {
                    return false;
                }

                m_engineToken = start.returnValue();
                if (m_stopped) {
                    auto stop = iface->xrayStopOwned(m_engineToken);
                    stop.waitForFinished();
                    m_engineToken = 0;
                    return false;
                }
                if (QVariant(m_rawConfig.value(configKey::killSwitchOption).toString()).toBool()) {
                    int tunIndex = -1;
                    for (const QNetworkInterface &network : QNetworkInterface::allInterfaces()) {
                        if (network.humanReadableName() == domainTunName) {
                            tunIndex = network.index();
                            break;
                        }
                    }

                    if (tunIndex < 0) {
                        return false;
                    }

                    QJsonObject firewall = m_rawConfig;
                    firewall.insert(configKey::splitTunnelType, RouteMode::VpnAllSites);
                    auto allow = iface->enableKillSwitch(firewall, tunIndex);
                    if (!allow.waitForFinished() || !allow.returnValue()) {
                        return false;
                    }
                }

                auto flush = iface->flushDns();
                return flush.waitForFinished() && flush.returnValue();
            },
            []() { return false; });
    if (m_stopped) {
        return;
    }
    if (!started) {
        fail(ErrorCode::XrayExecutableCrashed);
        return;
    }

    m_vpnLocalAddress = m_options.tunnel.sourceAddress;
    m_vpnGateway = m_wireguard->vpnGateway();
    m_engineTimer.start();
    if (m_tunnelConnected) {
        stopTimeoutTimer();
        setConnectionState(Vpn::ConnectionState::Connected);
    } else {
        if (!m_handshakeProbe.bind(QHostAddress(m_options.tunnel.sourceAddress), 0)) {
            fail(ErrorCode::DomainRoutingConfigInvalid);
            return;
        }
        DWORD index = htonl(static_cast<DWORD>(tunnel.index()));
        if (setsockopt(static_cast<SOCKET>(m_handshakeProbe.socketDescriptor()), IPPROTO_IP, IP_UNICAST_IF,
                       reinterpret_cast<const char *>(&index), sizeof(index))
            != 0) {
            fail(ErrorCode::DomainRoutingConfigInvalid);
            return;
        }
        const QByteArray query = QByteArray::fromHex("0001010000010000000000000000020001");
        if (m_handshakeProbe.writeDatagram(query, QHostAddress(m_options.tunnel.dnsServers.first()), 53) != query.size()) {
            fail(ErrorCode::DomainRoutingConfigInvalid);
        }
    }
}

void DomainRoutingProtocol::checkEngine()
{
    if (m_stopped) {
        return;
    }

    bool pathAvailable = false;
    for (const QNetworkInterface &network : QNetworkInterface::allInterfaces()) {
        if (network.humanReadableName() != m_options.direct.interfaceName
            || !network.flags().testFlag(QNetworkInterface::IsUp)) {
            continue;
        }

        for (const QNetworkAddressEntry &entry : network.addressEntries()) {
            if (entry.ip().toString() == m_options.direct.sourceAddress
                && interfaceDns(network.index()) == m_options.direct.dnsServers) {
                pathAvailable = true;
                break;
            }
        }
    }
    if (!pathAvailable) {
        fail(ErrorCode::DomainRoutingConfigInvalid);
        return;
    }

    const bool running = IpcClient::withInterface(
            [this](QSharedPointer<IpcInterfaceReplica> iface) {
                auto alive = iface->xrayKeepAliveOwned(m_engineToken);
                return alive.waitForFinished() && alive.returnValue();
            },
            []() { return false; });
    if (!running) {
        fail(ErrorCode::XrayExecutableCrashed);
    }
}

void DomainRoutingProtocol::fail(ErrorCode error)
{
    if (m_stopped) {
        return;
    }

    stop();
    setLastError(error);
    emit protocolError(error);
}

void DomainRoutingProtocol::stop()
{
    if (guardStop()) {
        return;
    }

    m_tunnelTimer.stop();
    m_engineTimer.stop();
    stopTimeoutTimer();
    m_handshakeProbe.close();
    if (m_engineToken != 0) {
        IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> iface) {
            auto stop = iface->xrayStopOwned(m_engineToken);
            stop.waitForFinished();
            auto flush = iface->flushDns();
            flush.waitForFinished();
        });
        m_engineToken = 0;
    }

    if (m_wireguard) {
        m_wireguard->stop();
    }

    setConnectionState(Vpn::ConnectionState::Disconnected);
}
