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

  m_handshakeTimer.setSingleShot(true);
  connect(&m_handshakeTimer, &QTimer::timeout, this, &Daemon::checkHandshake);
}

Daemon::~Daemon() {
  MZ_COUNT_DTOR(Daemon);

  logger.debug() << "Daemon released";

  Q_ASSERT(s_daemon == this);
  s_daemon = nullptr;
}

// static
Daemon* Daemon::instance() {
  Q_ASSERT(s_daemon);
  return s_daemon;
}

bool Daemon::activate(const InterfaceConfig& config) {
  Q_ASSERT(wgutils() != nullptr);

  // There are 3 possible scenarios in which this method is called:
  //
  // 1. the VPN is off: the method tries to enable the VPN.
  // 2. the VPN is on and the platform doesn't support the server-switching:
  //    this method calls deactivate() and then it continues as 1.
  // 3. the VPN is on and the platform supports the server-switching: this
  //    method calls switchServer().
  //
  // At the end, if the activation succeds, the `connected` signal is emitted.
  // If the activation abort's for any reason `the `activationFailure` signal is
  // emitted.
  logger.debug() << "Activating interface";
  auto emit_failure_guard = qScopeGuard([this] { emit activationFailure(); });

  if (m_connections.contains(config.m_hopType)) {
    if (supportServerSwitching(config)) {
      logger.debug() << "Already connected. Server switching supported.";

      if (!switchServer(config)) {
        return false;
      }

      if (!dnsutils()->restoreResolvers()) {
        return false;
      }

      if (!maybeUpdateResolvers(config)) {
        return false;
      }

      bool status = run(Switch, config);
      logger.debug() << "Connection status:" << status;
      if (status) {
        m_connections[config.m_hopType] = ConnectionState(config);
        m_handshakeTimer.start(HANDSHAKE_POLL_MSEC);
        emit_failure_guard.dismiss();
        return true;
      }
      return false;
    }

    logger.warning() << "Already connected. Server switching not supported.";
    if (!deactivate(false)) {
      return false;
    }

    Q_ASSERT(!m_connections.contains(config.m_hopType));
    if (activate(config)) {
      emit_failure_guard.dismiss();
      return true;
    }
    return false;
  }

  prepareActivation(config);

  // Bring up the wireguard interface if not already done.
  if (!wgutils()->interfaceExists()) {
    if (!wgutils()->addInterface(config)) {
      logger.error() << "Interface creation failed.";
      return false;
    }
  }

  // Configure routing for excluded addresses.
  for (const QString& i : config.m_excludedAddresses) {
    addExclusionRoute(IPAddress(i));
  }

  // Add the peer to this interface.
  if (!wgutils()->updatePeer(config)) {
    logger.error() << "Peer creation failed.";
    return false;
  }

  if (!maybeUpdateResolvers(config)) {
    return false;
  }

  if (supportIPUtils()) {
    if (!iputils()->addInterfaceIPs(config)) {
      return false;
    }
    if (!iputils()->setMTUAndUp(config)) {
      return false;
    }
  }

  // set routing
  for (const IPAddress& ip : config.m_allowedIPAddressRanges) {
    if (!wgutils()->updateRoutePrefix(ip)) {
      logger.debug() << "Routing configuration failed for"
                     << logger.sensitive(ip.toString());
      return false;
    }
  }

  bool status = run(Up, config);
  logger.debug() << "Connection status:" << status;
  if (status) {
    m_connections[config.m_hopType] = ConnectionState(config);
    m_handshakeTimer.start(HANDSHAKE_POLL_MSEC);
    emit_failure_guard.dismiss();
    return true;
  }
  return false;
}

bool Daemon::maybeUpdateResolvers(const InterfaceConfig& config) {
  if ((config.m_hopType == InterfaceConfig::MultiHopExit) ||
      (config.m_hopType == InterfaceConfig::SingleHop)) {
    QList<QHostAddress> resolvers;
    resolvers.append(QHostAddress(config.m_dnsServer));

    // If the DNS is not the Gateway, it's a user defined DNS
    // thus, not add any other :)
    if (config.m_dnsServer == config.m_serverIpv4Gateway) {
      resolvers.append(QHostAddress(config.m_serverIpv6Gateway));
    }

    if (!dnsutils()->updateResolvers(wgutils()->interfaceName(), resolvers)) {
      return false;
    }
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

bool Daemon::addExclusionRoute(const IPAddress& prefix) {
  if (m_excludedAddrSet.contains(prefix)) {
    m_excludedAddrSet[prefix]++;
    return true;
  }
  if (!wgutils()->addExclusionRoute(prefix)) {
    return false;
  }
  m_excludedAddrSet[prefix] = 1;
  return true;
}

bool Daemon::delExclusionRoute(const IPAddress& prefix) {
  Q_ASSERT(m_excludedAddrSet.contains(prefix));
  if (m_excludedAddrSet[prefix] > 1) {
    m_excludedAddrSet[prefix]--;
    return true;
  }
  m_excludedAddrSet.remove(prefix);
  return wgutils()->deleteExclusionRoute(prefix);
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

  if (!obj.contains("dnsServer")) {
    config.m_dnsServer = QString();
  } else {
    QJsonValue value = obj.value("dnsServer");
    if (!value.isString()) {
      logger.error() << "dnsServer is not a string";
      return false;
    }
    config.m_dnsServer = value.toString();
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

  return true;
}

bool Daemon::deactivate(bool emitSignals) {
  Q_ASSERT(wgutils() != nullptr);

  // Deactivate the main interface.
  if (!m_connections.isEmpty()) {
    const ConnectionState& state = m_connections.first();
    if (!run(Down, state.m_config)) {
      return false;
    }
  }

  if (emitSignals) {
    emit disconnected();
  }

  // Cleanup DNS
  if (!dnsutils()->restoreResolvers()) {
    logger.warning() << "Failed to restore DNS resolvers.";
  }

  // Cleanup peers and routing
  for (const ConnectionState& state : m_connections) {
    const InterfaceConfig& config = state.m_config;
    logger.debug() << "Deleting routes for" << config.m_hopType;
    for (const IPAddress& ip : config.m_allowedIPAddressRanges) {
      wgutils()->deleteRoutePrefix(ip);
    }
    wgutils()->deletePeer(config);
  }

  // Cleanup routing for excluded addresses.
  for (auto iterator = m_excludedAddrSet.constBegin();
       iterator != m_excludedAddrSet.constEnd(); ++iterator) {
    wgutils()->deleteExclusionRoute(iterator.key());
  }
  m_excludedAddrSet.clear();

  m_connections.clear();
  // Delete the interface
  return wgutils()->deleteInterface();  
}

QString Daemon::logs() {
  return {};
}

void Daemon::cleanLogs() { }

bool Daemon::supportServerSwitching(const InterfaceConfig& config) const {
  if (!m_connections.contains(config.m_hopType)) {
    return false;
  }
  const InterfaceConfig& current =
      m_connections.value(config.m_hopType).m_config;

  return current.m_privateKey == config.m_privateKey &&
         current.m_deviceIpv4Address == config.m_deviceIpv4Address &&
         current.m_deviceIpv6Address == config.m_deviceIpv6Address &&
         current.m_serverIpv4Gateway == config.m_serverIpv4Gateway &&
         current.m_serverIpv6Gateway == config.m_serverIpv6Gateway;
}

bool Daemon::switchServer(const InterfaceConfig& config) {
  Q_ASSERT(wgutils() != nullptr);

  logger.debug() << "Switching server for" << config.m_hopType;

  Q_ASSERT(m_connections.contains(config.m_hopType));
  const InterfaceConfig& lastConfig =
      m_connections.value(config.m_hopType).m_config;

  // Configure routing for new excluded addresses.
  for (const QString& i : config.m_excludedAddresses) {
    addExclusionRoute(IPAddress(i));
  }

  // Activate the new peer and its routes.
  if (!wgutils()->updatePeer(config)) {
    logger.error() << "Server switch failed to update the wireguard interface";
    return false;
  }
  for (const IPAddress& ip : config.m_allowedIPAddressRanges) {
    if (!wgutils()->updateRoutePrefix(ip)) {
      logger.error() << "Server switch failed to update the routing table";
      break;
    }
  }

  // Remove routing entries for the old peer.
  for (const QString& i : lastConfig.m_excludedAddresses) {
    delExclusionRoute(QHostAddress(i));
  }
  for (const IPAddress& ip : lastConfig.m_allowedIPAddressRanges) {
    if (!config.m_allowedIPAddressRanges.contains(ip)) {
      wgutils()->deleteRoutePrefix(ip);
    }
  }

  // Remove the old peer if it is no longer necessary.
  if (config.m_serverPublicKey != lastConfig.m_serverPublicKey) {
    if (!wgutils()->deletePeer(lastConfig)) {
      return false;
    }
  }

  m_connections[config.m_hopType] = ConnectionState(config);
  return true;
}

QJsonObject Daemon::getStatus() {
  Q_ASSERT(wgutils() != nullptr);
  QJsonObject json;
  logger.debug() << "Status request";

  if (!wgutils()->interfaceExists() || m_connections.isEmpty()) {
    json.insert("connected", QJsonValue(false));
    return json;
  }

  const ConnectionState& connection = m_connections.first();
  QList<WireguardUtils::PeerStatus> peers = wgutils()->getPeerStatus();
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

void Daemon::checkHandshake() {
  Q_ASSERT(wgutils() != nullptr);

  logger.debug() << "Checking for handshake...";

  int pendingHandshakes = 0;
  QList<WireguardUtils::PeerStatus> peers = wgutils()->getPeerStatus();
  for (ConnectionState& connection : m_connections) {
    const InterfaceConfig& config = connection.m_config;
    if (connection.m_date.isValid()) {
      continue;
    }
    logger.debug() << "awaiting" << config.m_serverPublicKey;

    // Check if the handshake has completed.
    for (const WireguardUtils::PeerStatus& status : peers) {
      if (config.m_serverPublicKey != status.m_pubkey) {
        continue;
      }
      if (status.m_handshake != 0) {
        connection.m_date.setMSecsSinceEpoch(status.m_handshake);
        emit connected(status.m_pubkey);
      }
    }

    if (!connection.m_date.isValid()) {
      pendingHandshakes++;
    }
  }

  // Check again if there were connections that haven't completed a handshake.
  if (pendingHandshakes > 0) {
    m_handshakeTimer.start(HANDSHAKE_POLL_MSEC);
  }
}
