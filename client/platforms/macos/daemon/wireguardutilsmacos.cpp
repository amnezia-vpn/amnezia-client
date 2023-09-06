/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "wireguardutilsmacos.h"

#include <errno.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QLocalSocket>
#include <QTimer>

#include "leakdetector.h"
#include "logger.h"

constexpr const int WG_TUN_PROC_TIMEOUT = 5000;
constexpr const char* WG_RUNTIME_DIR = "/var/run/wireguard";

namespace {
Logger logger("WireguardUtilsMacos");
Logger logwireguard("WireguardGo");
};  // namespace

WireguardUtilsMacos::WireguardUtilsMacos(QObject* parent)
    : WireguardUtils(parent), m_tunnel(this) {
  MZ_COUNT_CTOR(WireguardUtilsMacos);
  logger.debug() << "WireguardUtilsMacos created.";

  connect(&m_tunnel, SIGNAL(readyReadStandardOutput()), this,
          SLOT(tunnelStdoutReady()));
  connect(&m_tunnel, SIGNAL(errorOccurred(QProcess::ProcessError)), this,
          SLOT(tunnelErrorOccurred(QProcess::ProcessError)));
}

WireguardUtilsMacos::~WireguardUtilsMacos() {
  MZ_COUNT_DTOR(WireguardUtilsMacos);
  logger.debug() << "WireguardUtilsMacos destroyed.";
}

void WireguardUtilsMacos::tunnelStdoutReady() {
  for (;;) {
    QByteArray line = m_tunnel.readLine();
    if (line.length() <= 0) {
      break;
    }
    logwireguard.debug() << QString::fromUtf8(line);
  }
}

void WireguardUtilsMacos::tunnelErrorOccurred(QProcess::ProcessError error) {
  logger.warning() << "Tunnel process encountered an error:" << error;
  emit backendFailure();
}

bool WireguardUtilsMacos::addInterface(const InterfaceConfig& config) {
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
  QString wgNameFile = wgRuntimeDir.filePath(QString(WG_INTERFACE) + ".name");
  pe.insert("WG_TUN_NAME_FILE", wgNameFile);
#ifdef MZ_DEBUG
  pe.insert("LOG_LEVEL", "debug");
#endif
  m_tunnel.setProcessEnvironment(pe);

  QDir appPath(QCoreApplication::applicationDirPath());
  appPath.cdUp();
  appPath.cd("Resources");
  appPath.cd("utils");
  QStringList wgArgs = {"-f", "utun"};
  m_tunnel.start(appPath.filePath("wireguard-go"), wgArgs);
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
  m_rtmonitor = new MacosRouteMonitor(m_ifname, this);

  // Send a UAPI command to configure the interface
  QString message("set=1\n");
  QByteArray privateKey = QByteArray::fromBase64(config.m_privateKey.toUtf8());
  QTextStream out(&message);
  out << "private_key=" << QString(privateKey.toHex()) << "\n";
  out << "replace_peers=true\n";
  int err = uapiErrno(uapiCommand(message));
  if (err != 0) {
    logger.error() << "Interface configuration failed:" << strerror(err);
  }
  return (err == 0);
}

bool WireguardUtilsMacos::deleteInterface() {
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
  return true;
}

// dummy implementations for now
bool WireguardUtilsMacos::updatePeer(const InterfaceConfig& config) {
  QByteArray publicKey =
      QByteArray::fromBase64(qPrintable(config.m_serverPublicKey));

  logger.debug() << "Configuring peer" << logger.keys(config.m_serverPublicKey)
                 << "via" << config.m_serverIpv4AddrIn;

  // Update/create the peer config
  QString message;
  QTextStream out(&message);
  out << "set=1\n";
  out << "public_key=" << QString(publicKey.toHex()) << "\n";
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

bool WireguardUtilsMacos::deletePeer(const InterfaceConfig& config) {
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

QList<WireguardUtils::PeerStatus> WireguardUtilsMacos::getPeerStatus() {
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

bool WireguardUtilsMacos::updateRoutePrefix(const IPAddress& prefix) {
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

bool WireguardUtilsMacos::deleteRoutePrefix(const IPAddress& prefix) {
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

bool WireguardUtilsMacos::addExclusionRoute(const IPAddress& prefix) {
  if (!m_rtmonitor) {
    return false;
  }
  return m_rtmonitor->addExclusionRoute(prefix);
}

bool WireguardUtilsMacos::deleteExclusionRoute(const IPAddress& prefix) {
  if (!m_rtmonitor) {
    return false;
  }
  return m_rtmonitor->deleteExclusionRoute(prefix);
}

QString WireguardUtilsMacos::uapiCommand(const QString& command) {
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
int WireguardUtilsMacos::uapiErrno(const QString& reply) {
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

QString WireguardUtilsMacos::waitForTunnelName(const QString& filename) {
  QTimer timeout;
  timeout.setSingleShot(true);
  timeout.start(WG_TUN_PROC_TIMEOUT);

  QFile file(filename);
  while ((m_tunnel.state() == QProcess::Running) && timeout.isActive()) {
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
      continue;
    }
    QString ifname = QString::fromLocal8Bit(file.readLine()).trimmed();
    file.close();

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
