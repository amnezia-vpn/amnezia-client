/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "daemon.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaEnum>
#include <QTimer>

#include "leakdetector.h"
#include "logger.h"

constexpr const char* JSON_ALLOWEDIPADDRESSRANGES = "allowedIPAddressRanges";
constexpr int HANDSHAKE_POLL_MSEC = 250;

namespace {

Logger logger("Daemon");

Daemon* s_daemon = nullptr;

}  // namespace

Daemon::Daemon(QObject* parent) : QObject(parent) {
  MZ_COUNT_CTOR(Daemon);

  logger.debug() << "Daemon created";

  Q_ASSERT(s_daemon == nullptr);
  s_daemon = this;

  m_activationTimer.setSingleShot(false);
  connect(&m_activationTimer, &QTimer::timeout, this, &Daemon::checkActivations);
}

Daemon::~Daemon() {
  MZ_COUNT_DTOR(Daemon);

  logger.debug() << "Daemon released";

  qDeleteAll(m_tunnels);
  m_tunnels.clear();

  Q_ASSERT(s_daemon == this);
  s_daemon = nullptr;
}

// static
Daemon* Daemon::instance() {
  Q_ASSERT(s_daemon);
  return s_daemon;
}

bool Daemon::activate(const QString& ifname, const InterfaceConfig& config) {
  logger.debug() << "Activating tunnel";

  WireguardUtils* wg = m_tunnels.value(ifname);
  if (!wg) {
    wg = createWgUtils();
    if (!wg) {
      logger.error() << "Failed to create wireguard utils.";
      return false;
    }
    m_tunnels.insert(ifname, wg);
  }
  if (m_primaryIfname.isEmpty()) {
    m_primaryIfname = ifname;
  }

  ConnectionState& cs = m_connections[ifname];
  cs.m_config = config;
  cs.m_date = QDateTime();
  cs.m_deadline = QDateTime::currentDateTime().addMSecs(ACTIVATION_TIMEOUT_MSEC);

  auto failure_guard = qScopeGuard([this, ifname] {
    deactivateTunnel(ifname);
  });

  prepareActivation(config);

  // Bring up the wireguard interface if not already done.
  if (!wg->interfaceExists()) {
    if (!wg->addInterface(config)) {
      logger.error() << "Interface creation failed.";
      return false;
    }
  }

  // Bring the interface up.
  if (supportIPUtils()) {
    if (!iputils()->addInterfaceIPs(config)) {
      return false;
    }
    if (!iputils()->setMTUAndUp(config)) {
      return false;
    }
  }

  // Add the peer to this interface.
  if (!wg->updatePeer(config)) {
    logger.error() << "Peer creation failed.";
    return false;
  }

  if (!m_activationTimer.isActive()) {
    m_activationTimer.start(HANDSHAKE_POLL_MSEC);
  }

  failure_guard.dismiss();
  return true;
}

bool Daemon::setPrimary(const QString& ifname, const InterfaceConfig& config) {
  WireguardUtils* wg = m_tunnels.value(ifname);
  if (!wg) {
    logger.error() << "setPrimary: no tunnel for" << ifname;
    return false;
  }
  logger.debug() << "setPrimary" << wg->interfaceName();

  const QString priorPrimary = m_primaryIfname;
  m_primaryIfname = ifname;

  auto failure_guard = qScopeGuard([this, ifname, priorPrimary] {
    deactivateTunnel(ifname);
    m_primaryIfname = priorPrimary;
  });

  if (!run(Up, config)) {
    return false;
  }

  m_connections[ifname].m_config = config;

  failure_guard.dismiss();
  return true;
}

bool Daemon::deactivateTunnel(const QString& ifname) {
  WireguardUtils* wg = m_tunnels.value(ifname);
  const ConnectionState cs = m_connections.value(ifname);
  const InterfaceConfig& config = cs.m_config;
  const bool wasPrimary = (ifname == m_primaryIfname);
  const bool isLastTunnel = wg && m_tunnels.size() == 1;

  if (wg) {
    logger.debug() << "deactivateTunnel" << wg->interfaceName();
    if (isLastTunnel) {
      for (const IPAddress& prefix : m_excludedAddrSet.keys()) {
        wg->deleteExclusionRoute(prefix);
      }
      m_excludedAddrSet.clear();
    }
    wg->deletePeer(config);
    wg->deleteInterface();
    m_tunnels.remove(ifname);
    delete wg;
  }

  m_connections.remove(ifname);
  if (wasPrimary) {
    m_primaryIfname.clear();
  }
  return true;
}

// static
bool Daemon::parseStringList(const QJsonObject& obj, const QString& name,
                             QStringList& list) {
  if (obj.contains(name)) {
    QJsonValue value = obj.value(name);
    if (!value.isArray()) {
      logger.error() << name << "is not an array";
      return false;
    }
    QJsonArray array = value.toArray();
    for (const QJsonValue& i : array) {
      if (!i.isString()) {
        logger.error() << name << "must contain only strings";
        return false;
      }
      list.append(i.toString());
    }
  }
  return true;
}

bool Daemon::addExclusionRoute(const QString &addr) {
  IPAddress prefix(addr);
  if (m_excludedAddrSet.contains(prefix)) {
    m_excludedAddrSet[prefix]++;
    return true;
  }
  WireguardUtils* wg = primaryWgutils();
  if (!wg || !wg->addExclusionRoute(prefix)) {
    return false;
  }
  m_excludedAddrSet[prefix] = 1;
  return true;
}

bool Daemon::delExclusionRoute(const QString &addr) {
  IPAddress prefix(addr);
  if (!m_excludedAddrSet.contains(prefix)) {
    return false;
  }
  if (m_excludedAddrSet[prefix] > 1) {
    m_excludedAddrSet[prefix]--;
    return true;
  }
  m_excludedAddrSet.remove(prefix);
  WireguardUtils* wg = primaryWgutils();
  return wg && wg->deleteExclusionRoute(prefix);
}

bool Daemon::addAllowedIp(const QString &ifname, const QString &prefix) {
  WireguardUtils* wg = wgutilsFor(ifname);
  return wg && wg->updateRoutePrefix(IPAddress(prefix));
}

bool Daemon::delAllowedIp(const QString &ifname, const QString &prefix) {
  WireguardUtils* wg = wgutilsFor(ifname);
  return wg && wg->deleteRoutePrefix(IPAddress(prefix));
}

bool Daemon::setTunnelResolvers(const QString &ifname, const QStringList &resolvers) {
  WireguardUtils* wg = wgutilsFor(ifname);
  if (!wg || !dnsutils()) {
    return false;
  }
  QList<QHostAddress> hostAddrs;
  for (const QString& r : resolvers) {
    hostAddrs.append(QHostAddress(r));
  }
  return dnsutils()->updateResolvers(wg->interfaceName(), hostAddrs);
}

bool Daemon::restoreTunnelResolvers() {
  return dnsutils() && dnsutils()->restoreResolvers();
}

// static
bool Daemon::parseConfig(const QJsonObject& obj, InterfaceConfig& config) {
#define GETVALUE(name, where, jsontype)                           \
  if (!obj.contains(name)) {                                      \
    logger.debug() << name << " missing in the jsonConfig input"; \
    return false;                                                 \
  } else {                                                        \
    QJsonValue value = obj.value(name);                           \
    if (value.type() != QJsonValue::jsontype) {                   \
      logger.error() << name << " is not a " #jsontype;           \
      return false;                                               \
    }                                                             \
    where = value.to##jsontype();                                 \
  }

  GETVALUE("privateKey", config.m_privateKey, String);
  GETVALUE("serverPublicKey", config.m_serverPublicKey, String);
  GETVALUE("serverPort", config.m_serverPort, Double);

  config.m_serverPskKey = obj.value("serverPskKey").toString();

  if (!obj.contains("deviceMTU") || obj.value("deviceMTU").toString().toInt() == 0)
  {
    config.m_deviceMTU = 1420;
  } else {
    config.m_deviceMTU = obj.value("deviceMTU").toString().toInt();
#ifdef Q_OS_WINDOWS
// For Windows min MTU value is 1280 (the smallest MTU legal with IPv6).
    if (config.m_deviceMTU < 1280) {
      config.m_deviceMTU = 1280;
    }
#endif
  }

  config.m_deviceIpv4Address = obj.value("deviceIpv4Address").toString();
  config.m_deviceIpv6Address = obj.value("deviceIpv6Address").toString();
  if (config.m_deviceIpv4Address.isNull() &&
      config.m_deviceIpv6Address.isNull()) {
    logger.warning() << "no device addresses found in jsonConfig input";
    return false;
  }
  config.m_serverIpv4AddrIn = obj.value("serverIpv4AddrIn").toString();
  config.m_serverIpv6AddrIn = obj.value("serverIpv6AddrIn").toString();
  if (config.m_serverIpv4AddrIn.isNull() &&
      config.m_serverIpv6AddrIn.isNull()) {
    logger.error() << "no server addresses found in jsonConfig input";
    return false;
  }
  config.m_serverIpv4Gateway = obj.value("serverIpv4Gateway").toString();
  config.m_serverIpv6Gateway = obj.value("serverIpv6Gateway").toString();

  if (!obj.contains("primaryDnsServer")) {
    config.m_primaryDnsServer = QString();
  } else {
    QJsonValue value = obj.value("primaryDnsServer");
    if (!value.isString()) {
      logger.error() << "dnsServer is not a string";
      return false;
    }
    config.m_primaryDnsServer = value.toString();
  }

  if (!obj.contains("secondaryDnsServer")) {
    config.m_secondaryDnsServer = QString();
  } else {
    QJsonValue value = obj.value("secondaryDnsServer");
    if (!value.isString()) {
      logger.error() << "dnsServer is not a string";
      return false;
    }
    config.m_secondaryDnsServer = value.toString();
  }

  if (!obj.contains("hopType")) {
    config.m_hopType = InterfaceConfig::SingleHop;
  } else {
    QJsonValue value = obj.value("hopType");
    if (!value.isString()) {
      logger.error() << "hopType is not a string";
      return false;
    }

    bool okay;
    QByteArray vdata = value.toString().toUtf8();
    QMetaEnum meta = QMetaEnum::fromType<InterfaceConfig::HopType>();
    config.m_hopType =
        InterfaceConfig::HopType(meta.keyToValue(vdata.constData(), &okay));
    if (!okay) {
      logger.error() << "hopType" << value.toString() << "is not valid";
      return false;
    }
  }

  if (!obj.contains(JSON_ALLOWEDIPADDRESSRANGES)) {
    logger.error() << JSON_ALLOWEDIPADDRESSRANGES
                   << "missing in the jsonconfig input";
    return false;
  } else {
    QJsonValue value = obj.value(JSON_ALLOWEDIPADDRESSRANGES);
    if (!value.isArray()) {
      logger.error() << JSON_ALLOWEDIPADDRESSRANGES << "is not an array";
      return false;
    }

    QJsonArray array = value.toArray();
    for (const QJsonValue& i : array) {
      if (!i.isObject()) {
        logger.error() << JSON_ALLOWEDIPADDRESSRANGES
                       << "must contain only objects";
        return false;
      }

      QJsonObject ipObj = i.toObject();

      QJsonValue address = ipObj.value("address");
      if (!address.isString()) {
        logger.error() << JSON_ALLOWEDIPADDRESSRANGES
                       << "objects must have a string address";
        return false;
      }

      QJsonValue range = ipObj.value("range");
      if (!range.isDouble()) {
        logger.error() << JSON_ALLOWEDIPADDRESSRANGES
                       << "object must have a numberic range";
        return false;
      }

      QJsonValue isIpv6 = ipObj.value("isIpv6");
      if (!isIpv6.isBool()) {
        logger.error() << JSON_ALLOWEDIPADDRESSRANGES
                       << "object must have a boolean isIpv6";
        return false;
      }

      config.m_allowedIPAddressRanges.append(
          IPAddress(QHostAddress(address.toString()), range.toInt()));
    }

    // Sort allowed IPs by decreasing prefix length.
    std::sort(config.m_allowedIPAddressRanges.begin(),
              config.m_allowedIPAddressRanges.end(),
              [&](const IPAddress& a, const IPAddress& b) -> bool {
                return a.prefixLength() > b.prefixLength();
              });
  }

  if (!parseStringList(obj, "excludedAddresses", config.m_excludedAddresses)) {
    return false;
  }
  if (!parseStringList(obj, "vpnDisabledApps", config.m_vpnDisabledApps)) {
    return false;
  }
  if (!parseStringList(obj, "allowedDnsServers", config.m_allowedDnsServers)) {
    return false;
  }

  config.m_killSwitchEnabled = QVariant(obj.value("killSwitchOption").toString()).toBool();

  if (!obj.value("Jc").isNull()) {
    config.m_junkPacketCount = obj.value("Jc").toString();
  }
  if (!obj.value("Jmin").isNull()) {
    config.m_junkPacketMinSize = obj.value("Jmin").toString();
  }
  if (!obj.value("Jmax").isNull()) {
    config.m_junkPacketMaxSize = obj.value("Jmax").toString();
  }
  if (!obj.value("S1").isNull()) {
    config.m_initPacketJunkSize = obj.value("S1").toString();
  }
  if (!obj.value("S2").isNull()) {
    config.m_responsePacketJunkSize = obj.value("S2").toString();
  }
  if (!obj.value("S3").isNull()) {
    config.m_cookieReplyPacketJunkSize = obj.value("S3").toString();
  }
  if (!obj.value("S4").isNull()) {
    config.m_transportPacketJunkSize = obj.value("S4").toString();
  }

  if (!obj.value("H1").isNull()) {
    config.m_initPacketMagicHeader = obj.value("H1").toString();
  }
  if (!obj.value("H2").isNull()) {
    config.m_responsePacketMagicHeader = obj.value("H2").toString();
  }
  if (!obj.value("H3").isNull()) {
    config.m_underloadPacketMagicHeader = obj.value("H3").toString();
  }
  if (!obj.value("H4").isNull()) {
    config.m_transportPacketMagicHeader = obj.value("H4").toString();
  }

  if (!obj.value("I1").isNull()) {
    config.m_specialJunk["I1"] = obj.value("I1").toString();
  }
  if (!obj.value("I2").isNull()) {
    config.m_specialJunk["I2"] = obj.value("I2").toString();
  }
  if (!obj.value("I3").isNull()) {
    config.m_specialJunk["I3"] = obj.value("I3").toString();
  }
  if (!obj.value("I4").isNull()) {
    config.m_specialJunk["I4"] = obj.value("I4").toString();
  }
  if (!obj.value("I5").isNull()) {
    config.m_specialJunk["I5"] = obj.value("I5").toString();
  }
  config.m_ifname = obj.value("ifname").toString();

  return true;
}

bool Daemon::deactivate(bool emitSignals) {
  const QString primary = m_primaryIfname;

  if (m_connections.contains(primary)) {
    if (!run(Down, m_connections.value(primary).m_config)) {
      return false;
    }
  }

  if (emitSignals) {
    emit disconnected();
  }

  const QStringList ifnames = m_tunnels.keys();
  for (const QString& ifname : ifnames) {
    if (ifname != primary) {
      deactivateTunnel(ifname);
    }
  }

  if (auto* wg = primaryWgutils()) {
    for (const IPAddress& prefix : m_excludedAddrSet.keys()) {
      wg->deleteExclusionRoute(prefix);
    }
  }
  m_excludedAddrSet.clear();

  if (m_tunnels.contains(primary)) {
    deactivateTunnel(primary);
  }

  m_activationTimer.stop();
  return true;
}

QString Daemon::logs() {
  return {};
}

void Daemon::cleanLogs() { }

QJsonObject Daemon::getStatus() {
  QJsonObject json;
  logger.debug() << "Status request";

  WireguardUtils* wg = primaryWgutils();
  if (!wg || !wg->interfaceExists() || !m_connections.contains(m_primaryIfname)) {
    json.insert("connected", QJsonValue(false));
    return json;
  }

  const ConnectionState& connection = m_connections.value(m_primaryIfname);
  QList<WireguardUtils::PeerStatus> peers = wg->getPeerStatus();
  for (const WireguardUtils::PeerStatus& status : peers) {
    if (status.m_pubkey != connection.m_config.m_serverPublicKey) {
      continue;
    }
    json.insert("connected", QJsonValue(true));
    json.insert("serverIpv4Gateway",
                QJsonValue(connection.m_config.m_serverIpv4Gateway));
    json.insert("deviceIpv4Address",
                QJsonValue(connection.m_config.m_deviceIpv4Address));
    json.insert("date", connection.m_date.toString());
    json.insert("txBytes", QJsonValue(status.m_txBytes));
    json.insert("rxBytes", QJsonValue(status.m_rxBytes));
    return json;
  }

  json.insert("connected", QJsonValue(false));
  return json;
}

void Daemon::checkActivations() {
  const QDateTime now = QDateTime::currentDateTime();
  QStringList timedOut;
  bool anyPending = false;

  for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
    const QString& ifname = it.key();
    ConnectionState& cs = it.value();
    if (cs.m_date.isValid()) {
      continue;  // already handshaked
    }
    logger.debug() << "awaiting" << cs.m_config.m_serverPublicKey;

    WireguardUtils* wg = m_tunnels.value(ifname);
    bool handshaked = false;
    if (wg) {
      for (const WireguardUtils::PeerStatus& status : wg->getPeerStatus()) {
        if (status.m_pubkey != cs.m_config.m_serverPublicKey) {
          continue;
        }
        if (status.m_handshake != 0) {
          cs.m_date.setMSecsSinceEpoch(status.m_handshake);
          emit tunnelConnected(ifname, status.m_pubkey);
          handshaked = true;
        }
      }
    }
    if (handshaked) {
      continue;
    }
    if (cs.m_deadline.isValid() && now > cs.m_deadline) {
      timedOut.append(ifname);
    } else {
      anyPending = true;
    }
  }

  for (const QString& ifname : timedOut) {
    logger.warning() << "Tunnel handshake timed out:" << m_tunnels.value(ifname)->interfaceName();
    emit tunnelHandshakeFailed(ifname);
    deactivateTunnel(ifname);
  }

  if (!anyPending) {
    m_activationTimer.stop();
  }
}
