#include "xrayprotocol.h"

#include "core/ipcclient.h"
#include "ipc.h"
#include "utilities.h"
#include "core/networkUtilities.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QtCore/qlogging.h>
#include <QtCore/qobjectdefs.h>
#include <QtCore/qprocess.h>
#include <QThread>

#ifdef Q_OS_WIN
    #include <winsock2.h>
    #include <Iphlpapi.h>
    #include <iptypes.h>
    #include <Ws2tcpip.h>
#endif

#ifdef Q_OS_MACOS
static const QString tunName = "utun22";
#else
static const QString tunName = "tun2";
#endif

namespace {
#ifdef Q_OS_LINUX
constexpr int kLinuxDirectBypassMark = 0x3211;
#endif

int transportOutboundIndex(const QJsonObject &xrayConfig)
{
    const QJsonArray outbounds = xrayConfig.value("outbounds").toArray();
    for (int i = 0; i < outbounds.size(); ++i) {
        const QJsonObject outbound = outbounds.at(i).toObject();
        const QJsonObject settings = outbound.value("settings").toObject();
        if (!settings.value("vnext").toArray().isEmpty()) {
            return i;
        }
    }

    return outbounds.isEmpty() ? -1 : 0;
}

QString defaultOutboundTag(const QJsonObject &xrayConfig)
{
    const QJsonArray outbounds = xrayConfig.value("outbounds").toArray();
    if (outbounds.isEmpty()) {
        return QString();
    }

    const QString tag = outbounds.at(0).toObject().value("tag").toString();
    return tag.isEmpty() ? QStringLiteral("<untagged>") : tag;
}

void reorderOutboundsToDefaultTag(QJsonObject &xrayConfig, const QString &tag)
{
    if (tag.isEmpty()) {
        return;
    }

    QJsonArray outbounds = xrayConfig.value("outbounds").toArray();
    if (outbounds.size() < 2) {
        return;
    }

    int taggedIndex = -1;
    for (int i = 0; i < outbounds.size(); ++i) {
        if (outbounds.at(i).toObject().value("tag").toString() == tag) {
            taggedIndex = i;
            break;
        }
    }

    if (taggedIndex <= 0) {
        return;
    }

    const QJsonValue preferredOutbound = outbounds.at(taggedIndex);
    outbounds.removeAt(taggedIndex);
    outbounds.prepend(preferredOutbound);
    xrayConfig.insert("outbounds", outbounds);
}

void alignManagedRoutingDefaultOutbound(QJsonObject &xrayConfig, Settings::RouteMode routeMode)
{
    const QJsonArray rules = xrayConfig.value("routing").toObject().value("rules").toArray();
    if (rules.isEmpty()) {
        return;
    }

    bool hasDirectRules = false;
    bool hasProxyRules = false;
    for (const QJsonValue &ruleValue : rules) {
        const QJsonObject rule = ruleValue.toObject();
        const QString outboundTag = rule.value("outboundTag").toString();
        // Only count rules with domain matchers as split-tunnel indicators.
        // IP-only rules (e.g. system DNS proxy rules) are not user split-tunnel
        // rules and must not affect the default-outbound heuristic.
        const bool hasMatchers = !rule.value("domain").toArray().isEmpty()
                              || !rule.value("domainSuffix").toArray().isEmpty();
        if (!hasMatchers) {
            continue;
        }

        if (outboundTag == QLatin1String("direct")) {
            hasDirectRules = true;
        } else if (outboundTag == QLatin1String("proxy")) {
            hasProxyRules = true;
        }
    }

    QString preferredDefaultTag = QStringLiteral("proxy");
    if (hasProxyRules && !hasDirectRules) {
        preferredDefaultTag = QStringLiteral("direct");
    } else if (hasDirectRules && !hasProxyRules) {
        preferredDefaultTag = QStringLiteral("proxy");
    } else if (hasProxyRules && hasDirectRules && routeMode == Settings::RouteMode::VpnOnlyForwardSites) {
        preferredDefaultTag = QStringLiteral("direct");
    }

    reorderOutboundsToDefaultTag(xrayConfig, preferredDefaultTag);
}

void rememberProcessSnippet(QString &buffer, const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    if (!buffer.isEmpty()) {
        buffer += "\n";
    }
    buffer += trimmed;

    static constexpr int maxChars = 4096;
    if (buffer.size() > maxChars) {
        buffer = buffer.right(maxChars);
    }
}

void forceResolvedOutboundAddress(QJsonObject &xrayConfig, const QString &resolvedAddress)
{
    if (resolvedAddress.isEmpty()) {
        return;
    }

    QJsonArray outbounds = xrayConfig.value("outbounds").toArray();
    for (int i = 0; i < outbounds.size(); ++i) {
        QJsonObject outbound = outbounds.at(i).toObject();
        QJsonObject settings = outbound.value("settings").toObject();
        QJsonArray vnext = settings.value("vnext").toArray();
        if (vnext.isEmpty()) {
            continue;
        }

        QJsonObject firstHop = vnext.at(0).toObject();
        const QString currentAddress = firstHop.value("address").toString();
        if (currentAddress.compare(resolvedAddress, Qt::CaseInsensitive) == 0) {
            return;
        }

        firstHop.insert("address", resolvedAddress);
        vnext.replace(0, firstHop);
        settings.insert("vnext", vnext);
        outbound.insert("settings", settings);
        outbounds.replace(i, outbound);
        xrayConfig.insert("outbounds", outbounds);
        qDebug() << "XrayProtocol: forcing outbound address to resolved IPv4" << resolvedAddress << "(was" << currentAddress << ")";
        return;
    }
}

QString shortDigest(const QString &value)
{
    if (value.isEmpty()) {
        return "<empty>";
    }

    const QByteArray digest = QCryptographicHash::hash(value.toUtf8(), QCryptographicHash::Sha1).toHex();
    return QString::fromLatin1(digest.left(10));
}

bool isTcpPortReachable(const QString &address, quint16 port, int timeoutMs = 1200)
{
    if (address.isEmpty() || port == 0) {
        return false;
    }

    QTcpSocket socket;
    socket.connectToHost(address, port);
    const bool connected = socket.waitForConnected(timeoutMs);
    if (connected) {
        socket.disconnectFromHost();
    }
    return connected;
}

bool isPrivateIPv4Address(const QString &address)
{
    const QHostAddress host(address);
    if (host.protocol() != QAbstractSocket::IPv4Protocol) {
        return false;
    }

    return host.isInSubnet(QHostAddress(QStringLiteral("10.0.0.0")), 8)
            || host.isInSubnet(QHostAddress(QStringLiteral("172.16.0.0")), 12)
            || host.isInSubnet(QHostAddress(QStringLiteral("192.168.0.0")), 16);
}

bool fallbackOutboundPortTo443(QJsonObject &xrayConfig, const QString &resolvedAddress)
{
    if (resolvedAddress.isEmpty()) {
        return false;
    }

    QJsonArray outbounds = xrayConfig.value("outbounds").toArray();
    const int outboundIndex = transportOutboundIndex(xrayConfig);
    if (outboundIndex < 0 || outboundIndex >= outbounds.size()) {
        return false;
    }

    QJsonObject outbound = outbounds.at(outboundIndex).toObject();
    QJsonObject settings = outbound.value("settings").toObject();
    QJsonArray vnext = settings.value("vnext").toArray();
    if (vnext.isEmpty()) {
        return false;
    }

    QJsonObject firstHop = vnext.at(0).toObject();
    const int configuredPort = firstHop.value("port").toInt();
    if (configuredPort <= 0) {
        return false;
    }

    if (isTcpPortReachable(resolvedAddress, static_cast<quint16>(configuredPort))) {
        qDebug() << "XrayProtocol: configured outbound port is reachable:" << resolvedAddress << configuredPort;
        return false;
    }

    if (configuredPort == 443) {
        qWarning() << "XrayProtocol: configured outbound port 443 is not reachable:" << resolvedAddress;
        return false;
    }

    if (!isTcpPortReachable(resolvedAddress, 443)) {
        qWarning() << "XrayProtocol: configured outbound port is unreachable and 443 fallback is also closed:"
                   << resolvedAddress << configuredPort;
        return false;
    }

    qWarning() << "XrayProtocol: outbound port" << configuredPort
               << "is unreachable; switching VLESS outbound to 443 for" << resolvedAddress;
    firstHop.insert("port", 443);
    vnext.replace(0, firstHop);
    settings.insert("vnext", vnext);
    outbound.insert("settings", settings);
    outbounds.replace(outboundIndex, outbound);
    xrayConfig.insert("outbounds", outbounds);
    return true;
}

quint16 firstOutboundPort(const QJsonObject &xrayConfig)
{
    const QJsonArray outbounds = xrayConfig.value("outbounds").toArray();
    const int outboundIndex = transportOutboundIndex(xrayConfig);
    if (outboundIndex < 0 || outboundIndex >= outbounds.size()) {
        return 0;
    }

    const QJsonObject outbound = outbounds.at(outboundIndex).toObject();
    const QJsonObject settings = outbound.value("settings").toObject();
    const QJsonArray vnext = settings.value("vnext").toArray();
    if (vnext.isEmpty()) {
        return 0;
    }

    return static_cast<quint16>(vnext.at(0).toObject().value("port").toInt());
}

void sanitizeDesktopXrayConfig(QJsonObject &xrayConfig)
{
    QJsonObject log = xrayConfig.value("log").toObject();
    log.insert("loglevel", "warning");
    xrayConfig.insert("log", log);

    QJsonArray inbounds = xrayConfig.value("inbounds").toArray();
    if (!inbounds.isEmpty()) {
        QJsonObject inbound = inbounds.at(0).toObject();
        QJsonObject settings = inbound.value("settings").toObject();
        settings.insert("auth", "noauth");
        settings.insert("udp", true);
        inbound.insert("settings", settings);
        inbounds.replace(0, inbound);
        xrayConfig.insert("inbounds", inbounds);
    }

    QJsonArray outbounds = xrayConfig.value("outbounds").toArray();
    for (int i = 0; i < outbounds.size(); ++i) {
        QJsonObject outbound = outbounds.at(i).toObject();
        QJsonObject streamSettings = outbound.value("streamSettings").toObject();
        QJsonObject sockopt = streamSettings.value("sockopt").toObject();
        const QString protocol = outbound.value("protocol").toString();
        const QString tag = outbound.value("tag").toString();

        if (protocol == QLatin1String("vless")) {
            sockopt.insert("tcpFastOpen", true);
            sockopt.insert("tcpKeepAliveIdle", 45);
            sockopt.insert("tcpKeepAliveInterval", 45);
        }

#ifdef Q_OS_LINUX
        // Linux kill switch allows "without VPN" traffic only when packets are
        // explicitly marked for the amnvpnrt bypass table. Xray direct/freedom
        // outbound sockets must therefore carry the same fwmark as the
        // firewall's allowPIA/tagPkts path.
        if (protocol == QLatin1String("freedom") || tag == QLatin1String("direct")) {
            sockopt.insert("mark", kLinuxDirectBypassMark);
        }
#endif

        if (sockopt.isEmpty()) {
            continue;
        }

        streamSettings.insert("sockopt", sockopt);
        outbound.insert("streamSettings", streamSettings);
        outbounds.replace(i, outbound);
    }
    xrayConfig.insert("outbounds", outbounds);
}

void logXrayConfigSummary(const QJsonObject &xrayConfig)
{
    const QJsonArray outbounds = xrayConfig.value("outbounds").toArray();
    const int transportIndex = transportOutboundIndex(xrayConfig);
    if (transportIndex < 0 || transportIndex >= outbounds.size()) {
        qWarning() << "XrayProtocol: no outbounds in config";
        return;
    }

    const QJsonObject outbound = outbounds.at(transportIndex).toObject();
    const QJsonObject settings = outbound.value("settings").toObject();
    const QJsonArray vnext = settings.value("vnext").toArray();
    const QJsonObject firstHop = vnext.isEmpty() ? QJsonObject() : vnext.at(0).toObject();
    const QJsonArray users = firstHop.value("users").toArray();
    const QJsonObject user = users.isEmpty() ? QJsonObject() : users.at(0).toObject();
    const QJsonObject streamSettings = outbound.value("streamSettings").toObject();
    const QJsonObject realitySettings = streamSettings.value("realitySettings").toObject();
    const QJsonObject routing = xrayConfig.value("routing").toObject();
    const QJsonArray routingRules = routing.value("rules").toArray();
    int directDomainRules = 0;
    int directIpRules = 0;
    int proxyDomainRules = 0;
    int proxyIpRules = 0;

    for (const QJsonValue &ruleValue : routingRules) {
        const QJsonObject rule = ruleValue.toObject();
        const QString outboundTag = rule.value("outboundTag").toString();
        const int domainCount = rule.value("domain").toArray().size();
        const int ipCount = rule.value("ip").toArray().size();

        if (outboundTag == QLatin1String("direct")) {
            directDomainRules += domainCount;
            directIpRules += ipCount;
        } else if (outboundTag == QLatin1String("proxy")) {
            proxyDomainRules += domainCount;
            proxyIpRules += ipCount;
        }
    }

    qDebug().noquote()
        << QString("XrayProtocol config summary: address=%1 port=%2 serverName=%3 network=%4 security=%5 flow=%6 shortIdHash=%7 publicKeyHash=%8 routingRules=%9 directDomains=%10 directIps=%11 proxyDomains=%12 proxyIps=%13 defaultOutbound=%14")
               .arg(firstHop.value("address").toString())
               .arg(firstHop.value("port").toInt())
               .arg(realitySettings.value("serverName").toString())
               .arg(streamSettings.value("network").toString())
               .arg(streamSettings.value("security").toString())
               .arg(user.value("flow").toString())
               .arg(shortDigest(realitySettings.value("shortId").toString()))
               .arg(shortDigest(realitySettings.value("publicKey").toString()))
               .arg(routingRules.size())
               .arg(directDomainRules)
               .arg(directIpRules)
               .arg(proxyDomainRules)
               .arg(proxyIpRules)
               .arg(defaultOutboundTag(xrayConfig));
}

#ifdef Q_OS_WIN
QStringList dnsServersForInterfaceIndex(ULONG interfaceIndex)
{
    QStringList resolvers;
    if (interfaceIndex == 0) {
        return resolvers;
    }

    ULONG bufferLength = 16 * 1024;
    PIP_ADAPTER_ADDRESSES addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(malloc(bufferLength));
    if (!addresses) {
        return resolvers;
    }

    DWORD result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, nullptr, addresses, &bufferLength);
    if (result == ERROR_BUFFER_OVERFLOW) {
        free(addresses);
        addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(malloc(bufferLength));
        if (!addresses) {
            return resolvers;
        }
        result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, nullptr, addresses, &bufferLength);
    }

    if (result != NO_ERROR) {
        free(addresses);
        return resolvers;
    }

    for (PIP_ADAPTER_ADDRESSES adapter = addresses; adapter != nullptr; adapter = adapter->Next) {
        if (adapter->IfIndex != interfaceIndex) {
            continue;
        }

        for (PIP_ADAPTER_DNS_SERVER_ADDRESS_XP dns = adapter->FirstDnsServerAddress; dns != nullptr; dns = dns->Next) {
            if (!dns->Address.lpSockaddr || dns->Address.lpSockaddr->sa_family != AF_INET) {
                continue;
            }

            char buffer[INET_ADDRSTRLEN] = {'\0'};
            const sockaddr_in *addr = reinterpret_cast<const sockaddr_in *>(dns->Address.lpSockaddr);
            if (inet_ntop(AF_INET, &(addr->sin_addr), buffer, sizeof(buffer)) != nullptr) {
                const QString ip = QString::fromLatin1(buffer);
                if (!ip.isEmpty() && !resolvers.contains(ip)) {
                    resolvers.append(ip);
                }
            }
        }
        break;
    }

    free(addresses);
    return resolvers;
}
#endif
}

XrayProtocol::XrayProtocol(const QJsonObject &configuration, QObject *parent) : VpnProtocol(configuration, parent)
{
    m_vpnGateway = fblink::protocols::xray::defaultLocalAddr;
    m_vpnLocalAddress = fblink::protocols::xray::defaultLocalAddr;
    m_routeGateway = NetworkUtilities::getGatewayAndIface().first;

    m_routeMode = static_cast<Settings::RouteMode>(configuration.value(fblink::config_key::splitTunnelType).toInt());
    m_remoteAddress = NetworkUtilities::getIPAddress(m_rawConfig.value(fblink::config_key::hostName).toString());

    const QString primaryDns = configuration.value(fblink::config_key::dns1).toString();
    m_usesInternalFBLinkDns = fblink::protocols::dns::isFBLinkDnsAddress(primaryDns);
    m_forceTunResolversOnWindows = m_usesInternalFBLinkDns || isPrivateIPv4Address(primaryDns);
    m_dnsServers.push_back(QHostAddress(primaryDns));
    if (!m_usesInternalFBLinkDns) {
        const QString secondaryDns = configuration.value(fblink::config_key::dns2).toString();
        if (!secondaryDns.isEmpty()) {
            m_dnsServers.push_back(QHostAddress(secondaryDns));
        }
    }

    QJsonObject xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::Xray)).toObject();
    if (xrayConfiguration.isEmpty()) {
        xrayConfiguration = configuration.value(ProtocolProps::key_proto_config_data(Proto::SSXray)).toObject();
    }

    // Keep the transport path pinned to the already resolved IPv4 address.
    // This avoids runtime re-resolution to an IPv6/alternate record after the full-tunnel routes are installed.
    forceResolvedOutboundAddress(xrayConfiguration, m_remoteAddress);
    alignManagedRoutingDefaultOutbound(xrayConfiguration, m_routeMode);
    m_remoteEndpointReachable = fallbackOutboundPortTo443(xrayConfiguration, m_remoteAddress)
                                || isTcpPortReachable(m_remoteAddress, firstOutboundPort(xrayConfiguration));
    m_hasInternalRoutingRules = !xrayConfiguration.value("routing").toObject().value("rules").toArray().isEmpty();
    sanitizeDesktopXrayConfig(xrayConfiguration);
    logXrayConfigSummary(xrayConfiguration);
    m_xrayConfig = xrayConfiguration;
}

XrayProtocol::~XrayProtocol()
{
    qDebug() << "XrayProtocol::~XrayProtocol()";
    XrayProtocol::stop();
}

ErrorCode XrayProtocol::start()
{
    qDebug() << "XrayProtocol::start()";

    if (!m_remoteEndpointReachable) {
        qCritical() << "XrayProtocol::start(): refusing to start because remote endpoint is unreachable"
                    << m_remoteAddress;
        return ErrorCode::XrayRemoteEndpointUnavailable;
    }

    return IpcClient::withInterface([&](QSharedPointer<IpcInterfaceReplica> iface) {
        auto deleteTun = iface->deleteTun(tunName);
        if (!deleteTun.waitForFinished() || !deleteTun.returnValue()) {
            qWarning() << "XrayProtocol::start(): failed to pre-clean stale tunnel routes";
        } else {
            qDebug() << "XrayProtocol::start(): pre-cleaned stale tunnel routes";
        }

        auto xrayStart = iface->xrayStart(QJsonDocument(m_xrayConfig).toJson());
        if (!xrayStart.waitForFinished() || !xrayStart.returnValue()) {
            qCritical() << "Failed to start xray";
            return ErrorCode::XrayExecutableCrashed;
        }
        return startTun2Socks();
    }, [] () {
        return ErrorCode::FBLinkServiceConnectionFailed;
    });
}

void XrayProtocol::stop()
{
    qDebug() << "XrayProtocol::stop()";

    IpcClient::withInterface([](QSharedPointer<IpcInterfaceReplica> iface) {
        auto disableKillSwitch = iface->disableKillSwitch();
        if (!disableKillSwitch.waitForFinished() || !disableKillSwitch.returnValue())
            qWarning() << "Failed to disable killswitch";

        auto StartRoutingIpv6 = iface->StartRoutingIpv6();
        if (!StartRoutingIpv6.waitForFinished() || !StartRoutingIpv6.returnValue())
            qWarning() << "Failed to start routing ipv6";

        auto restoreResolvers = iface->restoreResolvers();
        if (!restoreResolvers.waitForFinished() || !restoreResolvers.returnValue())
            qWarning() << "Failed to restore resolvers";

        auto deleteTun = iface->deleteTun(tunName);
        if (!deleteTun.waitForFinished() || !deleteTun.returnValue())
            qWarning() << "Failed to delete tun";

        auto xrayStop = iface->xrayStop();
        if (!xrayStop.waitForFinished() || !xrayStop.returnValue())
            qWarning() << "Failed to stop xray";
    });

    if (m_tun2socksProcess) {
        m_tun2socksProcess->blockSignals(true);

#ifndef Q_OS_WIN
        m_tun2socksProcess->terminate();
        auto waitForFinished = m_tun2socksProcess->waitForFinished(1000);
        if (!waitForFinished.waitForFinished() || !waitForFinished.returnValue()) {
            qWarning() << "Failed to terminate tun2socks. Killing the process...";
            m_tun2socksProcess->kill();
        }
#else
        // terminate does not do anything useful on Windows
        // so just kill the process
        m_tun2socksProcess->kill();
#endif

        m_tun2socksProcess->close();
        m_tun2socksProcess.reset();
    }

    setConnectionState(Vpn::ConnectionState::Disconnected);
}

ErrorCode XrayProtocol::startTun2Socks()
{
    m_lastTun2SocksStdout.clear();
    m_lastTun2SocksStderr.clear();

    m_tun2socksProcess = IpcClient::CreatePrivilegedProcess();
    if (!m_tun2socksProcess->waitForSource()) {
        return ErrorCode::FBLinkServiceConnectionFailed;
    }

    m_tun2socksProcess->setProgram(PermittedProcess::Tun2Socks);
    m_tun2socksProcess->setArguments({"-device", QString("tun://%1").arg(tunName), "-proxy", "socks5://127.0.0.1:10808" });

    connect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardOutput, this, [this]() {
        auto readAllStandardOutput = m_tun2socksProcess->readAllStandardOutput();
        if (!readAllStandardOutput.waitForFinished()) {
            qWarning() << "Failed to read output from tun2socks";
            return;
        }

        const QString line = readAllStandardOutput.returnValue();
        rememberProcessSnippet(m_lastTun2SocksStdout, line);

        if (!line.contains("[TCP]") && !line.contains("[UDP]"))
            qDebug() << "[tun2socks]:" << line;
        
        if (line.contains("[STACK] tun://") && line.contains("<-> socks5://127.0.0.1")) {
            disconnect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardOutput, this, nullptr);

            if (ErrorCode res = setupRouting(); res != ErrorCode::NoError) {
                stop();
                setLastError(res);
            } else {
                setConnectionState(Vpn::ConnectionState::Connected);
            }
        }
    }, Qt::QueuedConnection);

    connect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::readyReadStandardError, this, [this]() {
        auto readAllStandardError = m_tun2socksProcess->readAllStandardError();
        if (!readAllStandardError.waitForFinished()) {
            qWarning() << "Failed to read stderr from tun2socks";
            return;
        }

        const QString stderrText = QString::fromUtf8(readAllStandardError.returnValue()).trimmed();
        if (!stderrText.isEmpty()) {
            rememberProcessSnippet(m_lastTun2SocksStderr, stderrText);
            qWarning() << "[tun2socks][stderr]:" << stderrText;
        }
    }, Qt::QueuedConnection);

    connect(m_tun2socksProcess.data(), &IpcProcessInterfaceReplica::finished, this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        auto readAllStandardOutput = m_tun2socksProcess->readAllStandardOutput();
        if (readAllStandardOutput.waitForFinished()) {
            rememberProcessSnippet(m_lastTun2SocksStdout, QString::fromUtf8(readAllStandardOutput.returnValue()));
        }

        auto readAllStandardError = m_tun2socksProcess->readAllStandardError();
        if (readAllStandardError.waitForFinished()) {
            rememberProcessSnippet(m_lastTun2SocksStderr, QString::fromUtf8(readAllStandardError.returnValue()));
        }

        if (exitStatus == QProcess::ExitStatus::CrashExit) {
            qCritical() << "Tun2socks process crashed!"
                        << "lastStdout=" << m_lastTun2SocksStdout
                        << "lastStderr=" << m_lastTun2SocksStderr;
        } else {
            qCritical() << QString("Tun2socks process was closed with %1 exit code").arg(exitCode)
                        << "lastStdout=" << m_lastTun2SocksStdout
                        << "lastStderr=" << m_lastTun2SocksStderr;
        }
        stop();
        setLastError(ErrorCode::Tun2SockExecutableCrashed);
    }, Qt::QueuedConnection);

    m_tun2socksProcess->start();
    return ErrorCode::NoError;
}

ErrorCode XrayProtocol::setupRouting() {
    return IpcClient::withInterface([this](QSharedPointer<IpcInterfaceReplica> iface) -> ErrorCode {
#ifdef Q_OS_WIN
        const int inetAdapterIndex = NetworkUtilities::AdapterIndexTo(QHostAddress(m_remoteAddress));
        const QStringList physicalDnsResolvers = dnsServersForInterfaceIndex(static_cast<ULONG>(qMax(inetAdapterIndex, 0)));
#endif
        bool tunReady = false;
        static constexpr int kMaxCreateTunAttempts = 8;
        for (int attempt = 1; attempt <= kMaxCreateTunAttempts; ++attempt) {
            auto createTun = iface->createTun(tunName, fblink::protocols::xray::defaultLocalAddr);
            if (createTun.waitForFinished() && createTun.returnValue()) {
                tunReady = true;
                break;
            }

            qWarning() << "Failed to assign IP address for TUN, attempt"
                       << attempt << "of" << kMaxCreateTunAttempts;

            // Give tun2socks/wintun time to settle before forcing adapter cleanup.
            // Aggressive cleanup on every failure can race with adapter creation.
            if (attempt == 4 || attempt == 6) {
                auto deleteTun = iface->deleteTun(tunName);
                deleteTun.waitForFinished();
            }

            if (attempt < kMaxCreateTunAttempts) {
                QThread::msleep(500);
            }
        }

        if (!tunReady) {
            qCritical() << "Failed to assign IP address for TUN";
            return ErrorCode::InternalError;
        }

#ifdef Q_OS_WIN
        if (m_forceTunResolversOnWindows) {
            auto updateResolvers = iface->updateResolvers(tunName, m_dnsServers);
            if (!updateResolvers.waitForFinished() || !updateResolvers.returnValue()) {
                qCritical() << "Failed to set XRay DNS resolvers for TUN on Windows";
                return ErrorCode::InternalError;
            }
            qDebug() << "XrayProtocol::setupRouting(): applied XRay DNS resolvers on Windows";
        } else {
            qDebug() << "XrayProtocol::setupRouting(): keeping system DNS configuration on Windows";
        }
#else
        auto updateResolvers = iface->updateResolvers(tunName, m_dnsServers);
        if (!updateResolvers.waitForFinished() || !updateResolvers.returnValue()) {
            qCritical() << "Failed to set DNS resolvers for TUN";
            return ErrorCode::InternalError;
        }
#endif

#ifdef Q_OS_WIN
        int vpnAdapterIndex = -1;
        QList<QNetworkInterface> netInterfaces = QNetworkInterface::allInterfaces();
        for (auto& netInterface : netInterfaces) {
            for (auto& address : netInterface.addressEntries()) {
                if (m_vpnLocalAddress == address.ip().toString())
                    vpnAdapterIndex = netInterface.index();
            }
        }
#else
        static const int vpnAdapterIndex = 0;
#endif
        const bool requestedKillSwitchEnabled = QVariant(m_rawConfig.value(config_key::killSwitchOption).toString()).toBool();
        bool killSwitchEnabled = requestedKillSwitchEnabled;
#ifdef Q_OS_WIN
        if (requestedKillSwitchEnabled) {
            qWarning() << "XrayProtocol::setupRouting(): skipping legacy Windows kill switch for XRay session";
            killSwitchEnabled = false;
        }
#endif
        if (killSwitchEnabled) {
            if (vpnAdapterIndex != -1) {
                QJsonObject config = m_rawConfig;
                config.insert("vpnServer", m_remoteAddress);

                auto enableKillSwitch = IpcClient::Interface()->enableKillSwitch(config, vpnAdapterIndex);
                if (!enableKillSwitch.waitForFinished() || !enableKillSwitch.returnValue()) {
                    qCritical() << "Failed to enable killswitch";
                    return ErrorCode::InternalError;
                }
            } else
                qWarning() << "Failed to get vpnAdapterIndex. Killswitch disabled";
        }

        bool requiresFullTunnelRoutes =
                m_routeMode == Settings::RouteMode::VpnAllSites || m_hasInternalRoutingRules;
        if (!m_hasInternalRoutingRules && m_routeMode != Settings::RouteMode::VpnAllSites) {
            qWarning() << "XrayProtocol::setupRouting(): split mode requested without embedded routing rules;"
                          " forcing full-tunnel routes to keep connectivity";
            requiresFullTunnelRoutes = true;
        }
        if (requiresFullTunnelRoutes) {
            static const QStringList subnets = { "1.0.0.0/8", "2.0.0.0/7", "4.0.0.0/6", "8.0.0.0/5", "16.0.0.0/4", "32.0.0.0/3", "64.0.0.0/2", "128.0.0.0/1" };

            auto routeAddList =  iface->routeAddList(m_vpnGateway, subnets);
            if (!routeAddList.waitForFinished() || routeAddList.returnValue() != subnets.count()) {
                qCritical() << "Failed to set routes for TUN";
                return ErrorCode::InternalError;
            }

            if (m_hasInternalRoutingRules && m_routeMode != Settings::RouteMode::VpnAllSites) {
                qDebug() << "XrayProtocol::setupRouting(): forcing full-tunnel routes because embedded XRay routing rules are active";
            }
        }

        // Keep the transport path to the XRay/VLESS server outside of the TUN.
        // Otherwise full-tunnel mode can blackhole the server connection itself.
        if (!m_remoteAddress.isEmpty() && NetworkUtilities::checkIPv4Format(m_remoteAddress) && !m_routeGateway.isEmpty()) {
            auto routeServerDirect = iface->routeAddList(m_routeGateway, QStringList() << m_remoteAddress);
            if (!routeServerDirect.waitForFinished() || routeServerDirect.returnValue() < 1) {
                qWarning() << "Failed to add direct route to XRay server" << m_remoteAddress << "via" << m_routeGateway;
            } else {
                qDebug() << "Added direct route to XRay server" << m_remoteAddress << "via" << m_routeGateway;
            }
        }

#ifdef Q_OS_WIN
        if (!m_forceTunResolversOnWindows && !physicalDnsResolvers.isEmpty() && !m_routeGateway.isEmpty()) {
            auto routeDnsDirect = iface->routeAddList(m_routeGateway, physicalDnsResolvers);
            if (!routeDnsDirect.waitForFinished() || routeDnsDirect.returnValue() < physicalDnsResolvers.size()) {
                qWarning() << "Failed to add direct routes to system DNS resolvers" << physicalDnsResolvers << "via" << m_routeGateway;
            } else {
                qDebug() << "Added direct routes to system DNS resolvers" << physicalDnsResolvers << "via" << m_routeGateway;
            }
        } else if (m_forceTunResolversOnWindows) {
            qDebug() << "XrayProtocol::setupRouting(): skipping physical DNS bypass because XRay DNS override is active";
        } else {
            qDebug() << "XrayProtocol::setupRouting(): no physical IPv4 DNS resolvers detected for direct bypass";
        }
#endif

        auto StopRoutingIpv6 = iface->StopRoutingIpv6();
        if (!StopRoutingIpv6.waitForFinished() || !StopRoutingIpv6.returnValue()) {
            qWarning() << "Failed to disable IPv6 routing; continuing without IPv6 block routes";
        }

#ifdef Q_OS_WIN
        const bool hasAppSplitTunnel = !m_rawConfig.value(config_key::splitTunnelApps).toArray().isEmpty();
        if (inetAdapterIndex != -1 && vpnAdapterIndex != -1) {
            QJsonObject config = m_rawConfig;
            config.insert("inetAdapterIndex", inetAdapterIndex);
            config.insert("vpnAdapterIndex", vpnAdapterIndex);
            config.insert("vpnGateway", m_vpnGateway);
            config.insert("vpnServer", m_remoteAddress);

            // XRay full-tunnel already has an explicit direct route to the server.
            // Only invoke legacy Windows peer-traffic plumbing when a feature still needs it.
            if (killSwitchEnabled || hasAppSplitTunnel) {
                auto enablePeerTraffic = iface->enablePeerTraffic(config);
                if (!enablePeerTraffic.waitForFinished() || !enablePeerTraffic.returnValue()) {
                    qCritical() << "Failed to enable peer traffic";
                    return ErrorCode::InternalError;
                }
            } else {
                qDebug() << "XrayProtocol::setupRouting(): skipping Windows peer traffic plumbing for pure XRay full-tunnel";
            }
        } else
            qWarning() << "Failed to get adapter indexes. Split-tunneling disabled";
#endif
        return ErrorCode::NoError;
    },
    [] () {
        return ErrorCode::FBLinkServiceConnectionFailed;
    });
}
