/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wireguardutilslinux.h"

#include <errno.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QLocalSocket>
#include <QTimer>
#include <QThread>

#include "linuxfirewall.h"
#include "leakdetector.h"
#include "logger.h"

constexpr const int WG_TUN_PROC_TIMEOUT = 5000;
constexpr const char* WG_RUNTIME_DIR = "/var/run/amneziawg";

namespace {
Logger logger("WireguardUtilsLinux");
Logger logwireguard("WireguardGo");
};  // namespace

WireguardUtilsLinux::WireguardUtilsLinux(QObject* parent)
    : WireguardUtils(parent), m_tunnel(this) {
    MZ_COUNT_CTOR(WireguardUtilsLinux);
    logger.debug() << "WireguardUtilsLinux created.";

    connect(&m_tunnel, SIGNAL(readyReadStandardOutput()), this,
            SLOT(tunnelStdoutReady()));
    connect(&m_tunnel, SIGNAL(errorOccurred(QProcess::ProcessError)), this,
            SLOT(tunnelErrorOccurred(QProcess::ProcessError)));
}

WireguardUtilsLinux::~WireguardUtilsLinux() {
    MZ_COUNT_DTOR(WireguardUtilsLinux);
    logger.debug() << "WireguardUtilsLinux destroyed.";
}

void WireguardUtilsLinux::tunnelStdoutReady() {
    for (;;) {
        QByteArray line = m_tunnel.readLine();
        if (line.length() <= 0) {
            break;
        }
        logwireguard.debug() << QString::fromUtf8(line);
    }
}

void WireguardUtilsLinux::tunnelErrorOccurred(QProcess::ProcessError error) {
    logger.warning() << "Tunnel process encountered an error:" << error;
    emit backendFailure();
}

bool WireguardUtilsLinux::addInterface(const InterfaceConfig& config) {
    Q_UNUSED(config);
    if (m_tunnel.state() != QProcess::NotRunning) {
        logger.warning() << "Unable to start: tunnel process already running";
        return false;
    }

    QDir wgRuntimeDir(WG_RUNTIME_DIR);
    if (!wgRuntimeDir.exists()) {
        wgRuntimeDir.mkpath(".");
    }

    QProcessEnvironment pe = QProcessEnvironment::systemEnvironment();
    QString wgNameFile = wgRuntimeDir.filePath(QString(WG_INTERFACE) + ".sock");
    pe.insert("WG_TUN_NAME_FILE", wgNameFile);
#ifdef MZ_DEBUG
    pe.insert("LOG_LEVEL", "debug");
#endif
    m_tunnel.setProcessEnvironment(pe);

    QDir appPath(QCoreApplication::applicationDirPath());
    QStringList wgArgs = {"-f", "amn0"};
    m_tunnel.start(appPath.filePath("../../client/bin/wireguard-go"), wgArgs);
    if (!m_tunnel.waitForStarted(WG_TUN_PROC_TIMEOUT)) {
        logger.error() << "Unable to start tunnel process due to timeout";
        m_tunnel.kill();
        return false;
    }

    m_ifname = waitForTunnelName(wgNameFile);
    if (m_ifname.isNull()) {
        logger.error() << "Unable to read tunnel interface name";
        m_tunnel.kill();
        return false;
    }
    logger.debug() << "Created wireguard interface" << m_ifname;

    // Start the routing table monitor.
    m_rtmonitor = new LinuxRouteMonitor(m_ifname, this);

    // Send a UAPI command to configure the interface
    QString message("set=1\n");
    QByteArray privateKey = QByteArray::fromBase64(config.m_privateKey.toUtf8());
    QTextStream out(&message);
    out << "private_key=" << QString(privateKey.toHex()) << "\n";
    out << "replace_peers=true\n";


    if (!config.m_junkPacketCount.isEmpty()) {
        out << "jc=" << config.m_junkPacketCount << "\n";
    }
    if (!config.m_junkPacketMinSize.isEmpty()) {
        out << "jmin=" << config.m_junkPacketMinSize << "\n";
    }
    if (!config.m_junkPacketMaxSize.isEmpty()) {
        out << "jmax=" << config.m_junkPacketMaxSize << "\n";
    }
    if (!config.m_initPacketJunkSize.isEmpty()) {
        out << "s1=" << config.m_initPacketJunkSize << "\n";
    }
    if (!config.m_responsePacketJunkSize.isEmpty()) {
        out << "s2=" << config.m_responsePacketJunkSize << "\n";
    }
    if (!config.m_initPacketMagicHeader.isEmpty()) {
        out << "h1=" << config.m_initPacketMagicHeader << "\n";
    }
    if (!config.m_responsePacketMagicHeader.isEmpty()) {
        out << "h2=" << config.m_responsePacketMagicHeader << "\n";
    }
    if (!config.m_underloadPacketMagicHeader.isEmpty()) {
        out << "h3=" << config.m_underloadPacketMagicHeader << "\n";
    }
    if (!config.m_transportPacketMagicHeader.isEmpty()) {
        out << "h4=" << config.m_transportPacketMagicHeader << "\n";
    }

    int err = uapiErrno(uapiCommand(message));
    if (err != 0) {
        logger.error() << "Interface configuration failed:" << strerror(err);
    } else {
        if (config.m_killSwitchEnabled) {
            FirewallParams params { };
            params.dnsServers.append(config.m_dnsServer);
            if (config.m_allowedIPAddressRanges.contains(IPAddress("0.0.0.0/0"))) {
                params.blockAll = true;
                if (config.m_excludedAddresses.size()) {
                    params.allowNets = true;
                    foreach (auto net, config.m_excludedAddresses) {
                        params.allowAddrs.append(net.toUtf8());
                    }
                }
            } else {
                params.blockNets = true;
                foreach (auto net, config.m_allowedIPAddressRanges) {
                    params.blockAddrs.append(net.toString());
                }
            }
            applyFirewallRules(params);
        }
    }

    return (err == 0);
}

bool WireguardUtilsLinux::deleteInterface() {
    if (m_rtmonitor) {
        delete m_rtmonitor;
        m_rtmonitor = nullptr;
    }

    if (m_tunnel.state() == QProcess::NotRunning) {
        return false;
    }

    // Attempt to terminate gracefully.
    m_tunnel.terminate();
    if (!m_tunnel.waitForFinished(WG_TUN_PROC_TIMEOUT)) {
        m_tunnel.kill();
        m_tunnel.waitForFinished(WG_TUN_PROC_TIMEOUT);
    }

    // Garbage collect.
    QDir wgRuntimeDir(WG_RUNTIME_DIR);
    QFile::remove(wgRuntimeDir.filePath(QString(WG_INTERFACE) + ".name"));

    // double-check + ensure our firewall is installed and enabled
    LinuxFirewall::uninstall();
    return true;
}

// dummy implementations for now
bool WireguardUtilsLinux::updatePeer(const InterfaceConfig& config) {
    QByteArray publicKey =
        QByteArray::fromBase64(qPrintable(config.m_serverPublicKey));

    QByteArray pskKey = QByteArray::fromBase64(qPrintable(config.m_serverPskKey));

    logger.debug() << "Configuring peer" << config.m_serverPublicKey << "via" << config.m_serverIpv4AddrIn;

    // Update/create the peer config
    QString message;
    QTextStream out(&message);
    out << "set=1\n";
    out << "public_key=" << QString(publicKey.toHex()) << "\n";
    if (!config.m_serverPskKey.isNull()) {
        out << "preshared_key=" << QString(pskKey.toHex()) << "\n";
    }
    if (!config.m_serverIpv4AddrIn.isNull()) {
        out << "endpoint=" << config.m_serverIpv4AddrIn << ":";
    } else if (!config.m_serverIpv6AddrIn.isNull()) {
        out << "endpoint=[" << config.m_serverIpv6AddrIn << "]:";
    } else {
        logger.warning() << "Failed to create peer with no endpoints";
        return false;
    }
    out << config.m_serverPort << "\n";

    out << "replace_allowed_ips=true\n";
    out << "persistent_keepalive_interval=" << WG_KEEPALIVE_PERIOD << "\n";
    for (const IPAddress& ip : config.m_allowedIPAddressRanges) {
        out << "allowed_ip=" << ip.toString() << "\n";
    }

    // Exclude the server address, except for multihop exit servers.
    if ((config.m_hopType != InterfaceConfig::MultiHopExit) &&
        (m_rtmonitor != nullptr)) {
        m_rtmonitor->addExclusionRoute(IPAddress(config.m_serverIpv4AddrIn));
        m_rtmonitor->addExclusionRoute(IPAddress(config.m_serverIpv6AddrIn));
    }

    int err = uapiErrno(uapiCommand(message));
    if (err != 0) {
        logger.error() << "Peer configuration failed:" << strerror(err);
    }
    return (err == 0);
}

bool WireguardUtilsLinux::deletePeer(const InterfaceConfig& config) {
    QByteArray publicKey =
        QByteArray::fromBase64(qPrintable(config.m_serverPublicKey));

    // Clear exclustion routes for this peer.
    if ((config.m_hopType != InterfaceConfig::MultiHopExit) &&
        (m_rtmonitor != nullptr)) {
        m_rtmonitor->deleteExclusionRoute(IPAddress(config.m_serverIpv4AddrIn));
        m_rtmonitor->deleteExclusionRoute(IPAddress(config.m_serverIpv6AddrIn));
    }

    QString message;
    QTextStream out(&message);
    out << "set=1\n";
    out << "public_key=" << QString(publicKey.toHex()) << "\n";
    out << "remove=true\n";

    int err = uapiErrno(uapiCommand(message));
    if (err != 0) {
        logger.error() << "Peer deletion failed:" << strerror(err);
    }
    return (err == 0);
}

QList<WireguardUtils::PeerStatus> WireguardUtilsLinux::getPeerStatus() {
    QString reply = uapiCommand("get=1");
    PeerStatus status;
    QList<PeerStatus> peerList;
    for (const QString& line : reply.split('\n')) {
        int eq = line.indexOf('=');
        if (eq <= 0) {
            continue;
        }
        QString name = line.left(eq);
        QString value = line.mid(eq + 1);

        if (name == "public_key") {
            if (!status.m_pubkey.isEmpty()) {
                peerList.append(status);
            }
            QByteArray pubkey = QByteArray::fromHex(value.toUtf8());
            status = PeerStatus(pubkey.toBase64());
        }

        if (name == "tx_bytes") {
            status.m_txBytes = value.toDouble();
        }
        if (name == "rx_bytes") {
            status.m_rxBytes = value.toDouble();
        }
        if (name == "last_handshake_time_sec") {
            status.m_handshake += value.toLongLong() * 1000;
        }
        if (name == "last_handshake_time_nsec") {
            status.m_handshake += value.toLongLong() / 1000000;
        }
    }
    if (!status.m_pubkey.isEmpty()) {
        peerList.append(status);
    }

    return peerList;
}

bool WireguardUtilsLinux::updateRoutePrefix(const IPAddress& prefix) {
    if (!m_rtmonitor) {
        return false;
    }
    if (prefix.prefixLength() > 0) {
        return m_rtmonitor->insertRoute(prefix);
    }

    // Ensure that we do not replace the default route.
    if (prefix.type() == QAbstractSocket::IPv4Protocol) {
        return m_rtmonitor->insertRoute(IPAddress("0.0.0.0/1")) &&
               m_rtmonitor->insertRoute(IPAddress("128.0.0.0/1"));
    }
    if (prefix.type() == QAbstractSocket::IPv6Protocol) {
        return m_rtmonitor->insertRoute(IPAddress("::/1")) &&
               m_rtmonitor->insertRoute(IPAddress("8000::/1"));
    }

    return false;
}

bool WireguardUtilsLinux::deleteRoutePrefix(const IPAddress& prefix) {
    if (!m_rtmonitor) {
        return false;
    }
    if (prefix.prefixLength() > 0) {
        return m_rtmonitor->insertRoute(prefix);
    }

    // Ensure that we do not replace the default route.
    if (prefix.type() == QAbstractSocket::IPv4Protocol) {
        return m_rtmonitor->deleteRoute(IPAddress("0.0.0.0/1")) &&
               m_rtmonitor->deleteRoute(IPAddress("128.0.0.0/1"));
    } else if (prefix.type() == QAbstractSocket::IPv6Protocol) {
        return m_rtmonitor->deleteRoute(IPAddress("::/1")) &&
               m_rtmonitor->deleteRoute(IPAddress("8000::/1"));
    } else {
        return false;
    }
}

bool WireguardUtilsLinux::addExclusionRoute(const IPAddress& prefix) {
    if (!m_rtmonitor) {
        return false;
    }
    return m_rtmonitor->addExclusionRoute(prefix);
}

bool WireguardUtilsLinux::deleteExclusionRoute(const IPAddress& prefix) {
    if (!m_rtmonitor) {
        return false;
    }
    return m_rtmonitor->deleteExclusionRoute(prefix);
}

bool WireguardUtilsLinux::excludeLocalNetworks(const QList<IPAddress>& routes) {
  if (!m_rtmonitor) {
    return false;
  }

  // Explicitly discard LAN traffic that makes its way into the tunnel. This
  // doesn't really exclude the LAN traffic, we just don't take any action to
  // overrule the routes of other interfaces.
  bool result = true;
  for (const auto& prefix : routes) {
    logger.error() << "Attempting to exclude:" << prefix.toString();
    if (!m_rtmonitor->insertRoute(prefix)) {
      result = false;
    }
  }

  // TODO: A kill switch would be nice though :)
  return result;
}

QString WireguardUtilsLinux::uapiCommand(const QString& command) {
    QLocalSocket socket;
    QTimer uapiTimeout;
    QDir wgRuntimeDir(WG_RUNTIME_DIR);
    QString wgSocketFile = wgRuntimeDir.filePath(m_ifname + ".sock");

    uapiTimeout.setSingleShot(true);
    uapiTimeout.start(WG_TUN_PROC_TIMEOUT);

    socket.connectToServer(wgSocketFile, QIODevice::ReadWrite);
    if (!socket.waitForConnected(WG_TUN_PROC_TIMEOUT)) {
        logger.error() << "QLocalSocket::waitForConnected() failed:"
                       << socket.errorString();
        return QString();
    }

    // Send the message to the UAPI socket.
    QByteArray message = command.toLocal8Bit();
    while (!message.endsWith("\n\n")) {
        message.append('\n');
    }
    socket.write(message);

    QByteArray reply;
    while (!reply.contains("\n\n")) {
        if (!uapiTimeout.isActive()) {
            logger.error() << "UAPI command timed out";
            return QString();
        }
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        reply.append(socket.readAll());
    }

    return QString::fromUtf8(reply).trimmed();
}

// static
int WireguardUtilsLinux::uapiErrno(const QString& reply) {
    for (const QString& line : reply.split("\n")) {
        int eq = line.indexOf('=');
        if (eq <= 0) {
            continue;
        }
        if (line.left(eq) == "errno") {
            return line.mid(eq + 1).toInt();
        }
    }
    return EINVAL;
}

QString WireguardUtilsLinux::waitForTunnelName(const QString& filename) {
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.start(WG_TUN_PROC_TIMEOUT);

    QFile file(filename);

    while ((m_tunnel.state() == QProcess::Running) && timeout.isActive()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QString ifname = "amn0";

        // Test-connect to the UAPI socket.
        QLocalSocket sock;
        QDir wgRuntimeDir(WG_RUNTIME_DIR);
        QString sockName = wgRuntimeDir.filePath(ifname + ".sock");
        sock.connectToServer(sockName, QIODevice::ReadWrite);
        if (sock.waitForConnected(100)) {
            return ifname;
        }
    }

    return QString();
}

void WireguardUtilsLinux::applyFirewallRules(FirewallParams& params)
{
    // double-check + ensure our firewall is installed and enabled
    if (!LinuxFirewall::isInstalled()) LinuxFirewall::install();

    // Note: rule precedence is handled inside IpTablesFirewall
    LinuxFirewall::ensureRootAnchorPriority();

    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("000.allowLoopback"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("100.blockAll"), params.blockAll);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("110.allowNets"), params.allowNets);
    LinuxFirewall::updateAllowNets(params.allowAddrs);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("120.blockNets"), params.blockNets);
    LinuxFirewall::updateBlockNets(params.blockAddrs);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("200.allowVPN"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv6, QStringLiteral("250.blockIPv6"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("290.allowDHCP"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("300.allowLAN"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("310.blockDNS"), true);
    LinuxFirewall::updateDNSServers(params.dnsServers);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::IPv4, QStringLiteral("320.allowDNS"), true);
    LinuxFirewall::setAnchorEnabled(LinuxFirewall::Both, QStringLiteral("400.allowPIA"), true);
}
