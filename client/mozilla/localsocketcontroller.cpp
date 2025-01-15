/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "localsocketcontroller.h"

#include <stdint.h>

#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStandardPaths>

#include "ipaddress.h"
#include "leakdetector.h"
#include "logger.h"
#include "models/server.h"
#include "daemon/daemonerrors.h"

#include "protocols/protocols_defs.h"

// How many times do we try to reconnect.
constexpr int MAX_CONNECTION_RETRY = 10;

// How long do we wait between one try and the next one.
constexpr int CONNECTION_RETRY_TIMER_MSEC = 500;

namespace {
Logger logger("LocalSocketController");
}

LocalSocketController::LocalSocketController() {
  MZ_COUNT_CTOR(LocalSocketController);

  m_socket = new QLocalSocket(this);
  connect(m_socket, &QLocalSocket::connected, this,
          &LocalSocketController::daemonConnected);
  connect(m_socket, &QLocalSocket::disconnected, this, 
          [&] { errorOccurred(QLocalSocket::PeerClosedError); });
  connect(m_socket, &QLocalSocket::errorOccurred, this,
          &LocalSocketController::errorOccurred);
  connect(m_socket, &QLocalSocket::readyRead, this,
          &LocalSocketController::readData);

  m_initializingTimer.setSingleShot(true);
  connect(&m_initializingTimer, &QTimer::timeout, this,
          &LocalSocketController::initializeInternal);
}

LocalSocketController::~LocalSocketController() {
  MZ_COUNT_DTOR(LocalSocketController);
}

void LocalSocketController::errorOccurred(
    QLocalSocket::LocalSocketError error) {
  logger.error() << "Error occurred:" << error;

  if (m_daemonState == eInitializing) {
    if (m_initializingRetry++ < MAX_CONNECTION_RETRY) {
      m_initializingTimer.start(CONNECTION_RETRY_TIMER_MSEC);
      return;
    }

    emit initialized(false, false, QDateTime());
  }

  qCritical() << "ControllerError";
  disconnectInternal();
}

void LocalSocketController::disconnectInternal() {
  // We're still eReady as the Deamon is alive
  // and can make a new connection.
  m_daemonState = eReady;
  m_initializingRetry = 0;
  m_initializingTimer.stop();
  emit disconnected();
}

void LocalSocketController::initialize(const Device* device, const Keys* keys) {
  logger.debug() << "Initializing";

  Q_UNUSED(device);
  Q_UNUSED(keys);

  Q_ASSERT(m_daemonState == eUnknown);
  m_initializingRetry = 0;

  initializeInternal();
}

void LocalSocketController::initializeInternal() {
  m_daemonState = eInitializing;

#ifdef MZ_WINDOWS
  QString path = "\\\\.\\pipe\\amneziavpn";
#else
  QString path = "/var/run/amneziavpn/daemon.socket";
  if (!QFileInfo::exists(path)) {
    path = "/tmp/amneziavpn.socket";
  }
#endif

  logger.debug() << "Connecting to:" << path;
  m_socket->connectToServer(path);
}

void LocalSocketController::daemonConnected() {
  logger.debug() << "Daemon connected";
  Q_ASSERT(m_daemonState == eInitializing);
  checkStatus();
}

void LocalSocketController::activate(const QJsonObject &rawConfig) {

  QString protocolName = rawConfig.value("protocol").toString();

  int splitTunnelType = rawConfig.value("splitTunnelType").toInt();
  QJsonArray splitTunnelSites = rawConfig.value("splitTunnelSites").toArray();

  int appSplitTunnelType = rawConfig.value(amnezia::config_key::appSplitTunnelType).toInt();
  QJsonArray splitTunnelApps = rawConfig.value(amnezia::config_key::splitTunnelApps).toArray();

  QJsonObject wgConfig = rawConfig.value(protocolName + "_config_data").toObject();

  QJsonObject json;
  json.insert("type", "activate");
  //  json.insert("hopindex", QJsonValue((double)hop.m_hopindex));
  json.insert("privateKey", wgConfig.value(amnezia::config_key::client_priv_key));
  json.insert("deviceIpv4Address", wgConfig.value(amnezia::config_key::client_ip));

  // set up IPv6 unique-local-address, ULA, with "fd00::/8" prefix, not globally routable.
  // this will be default IPv6 gateway, OS recognizes that IPv6 link is local and switches to IPv4.
  // Otherwise some OSes (Linux) try IPv6 forever and hang. 
  // https://en.wikipedia.org/wiki/Unique_local_address (RFC 4193)
  // https://man7.org/linux/man-pages/man5/gai.conf.5.html
  json.insert("deviceIpv6Address", "fd58:baa6:dead::1"); // simply "dead::1" is globally-routable, don't use it

  json.insert("serverPublicKey", wgConfig.value(amnezia::config_key::server_pub_key));
  json.insert("serverPskKey", wgConfig.value(amnezia::config_key::psk_key));
  json.insert("serverIpv4AddrIn", wgConfig.value(amnezia::config_key::hostName));
  //  json.insert("serverIpv6AddrIn", QJsonValue(hop.m_server.ipv6AddrIn()));
  json.insert("deviceMTU", wgConfig.value(amnezia::config_key::mtu));

  json.insert("serverPort", wgConfig.value(amnezia::config_key::port).toInt());
  json.insert("serverIpv4Gateway", wgConfig.value(amnezia::config_key::hostName));
  //  json.insert("serverIpv6Gateway", QJsonValue(hop.m_server.ipv6Gateway()));
  json.insert("dnsServer", rawConfig.value(amnezia::config_key::dns1));

  QJsonArray jsAllowedIPAddesses;

  QJsonArray plainAllowedIP = wgConfig.value(amnezia::config_key::allowed_ips).toArray();
  QJsonArray defaultAllowedIP = { "0.0.0.0/0", "::/0" };

  if (plainAllowedIP != defaultAllowedIP && !plainAllowedIP.isEmpty()) {
    // Use AllowedIP list from WG config because of higher priority
    for (auto v : plainAllowedIP) {
      QString ipRange = v.toString();
      if (ipRange.split('/').size() > 1){
          QJsonObject range;
          range.insert("address", ipRange.split('/')[0]);
          range.insert("range", atoi(ipRange.split('/')[1].toLocal8Bit()));
          range.insert("isIpv6", false);
          jsAllowedIPAddesses.append(range);
      } else {
          QJsonObject range;
          range.insert("address",ipRange);
          range.insert("range", 32);
          range.insert("isIpv6", false);
          jsAllowedIPAddesses.append(range);
      }
    }
  } else {

    // Use APP split tunnel
      if (splitTunnelType == 0 || splitTunnelType == 2) {
          QJsonObject range_ipv4;
          range_ipv4.insert("address", "0.0.0.0");
          range_ipv4.insert("range", 0);
          range_ipv4.insert("isIpv6", false);
          jsAllowedIPAddesses.append(range_ipv4);

          QJsonObject range_ipv6;
          range_ipv6.insert("address", "::");
          range_ipv6.insert("range", 0);
          range_ipv6.insert("isIpv6", true);
          jsAllowedIPAddesses.append(range_ipv6);
      }

      if (splitTunnelType == 1) {
          for (auto v : splitTunnelSites) {
              QString ipRange = v.toString();
              if (ipRange.split('/').size() > 1){
                  QJsonObject range;
                  range.insert("address", ipRange.split('/')[0]);
                  range.insert("range", atoi(ipRange.split('/')[1].toLocal8Bit()));
                  range.insert("isIpv6", false);
                  jsAllowedIPAddesses.append(range);
              } else {
                  QJsonObject range;
                  range.insert("address",ipRange);
                  range.insert("range", 32);
                  range.insert("isIpv6", false);
                  jsAllowedIPAddesses.append(range);
              }
          }
      }
  }

  json.insert("allowedIPAddressRanges", jsAllowedIPAddesses);


  QJsonArray jsExcludedAddresses;
  jsExcludedAddresses.append(wgConfig.value(amnezia::config_key::hostName));
  if (splitTunnelType == 2) {
    for (auto v : splitTunnelSites) {
          QString ipRange = v.toString();
          jsExcludedAddresses.append(ipRange);
      }
  }

  json.insert("excludedAddresses", jsExcludedAddresses);

  json.insert("vpnDisabledApps", splitTunnelApps);

  json.insert(amnezia::config_key::killSwitchOption, rawConfig.value(amnezia::config_key::killSwitchOption));

  if (protocolName == amnezia::config_key::awg) {
    json.insert(amnezia::config_key::junkPacketCount, wgConfig.value(amnezia::config_key::junkPacketCount));
    json.insert(amnezia::config_key::junkPacketMinSize, wgConfig.value(amnezia::config_key::junkPacketMinSize));
    json.insert(amnezia::config_key::junkPacketMaxSize, wgConfig.value(amnezia::config_key::junkPacketMaxSize));
    json.insert(amnezia::config_key::initPacketJunkSize, wgConfig.value(amnezia::config_key::initPacketJunkSize));
    json.insert(amnezia::config_key::responsePacketJunkSize, wgConfig.value(amnezia::config_key::responsePacketJunkSize));
    json.insert(amnezia::config_key::initPacketMagicHeader, wgConfig.value(amnezia::config_key::initPacketMagicHeader));
    json.insert(amnezia::config_key::responsePacketMagicHeader, wgConfig.value(amnezia::config_key::responsePacketMagicHeader));
    json.insert(amnezia::config_key::underloadPacketMagicHeader, wgConfig.value(amnezia::config_key::underloadPacketMagicHeader));
    json.insert(amnezia::config_key::transportPacketMagicHeader, wgConfig.value(amnezia::config_key::transportPacketMagicHeader));
  } else if (!wgConfig.value(amnezia::config_key::junkPacketCount).isUndefined()
             && !wgConfig.value(amnezia::config_key::junkPacketMinSize).isUndefined()
             && !wgConfig.value(amnezia::config_key::junkPacketMaxSize).isUndefined()
             && !wgConfig.value(amnezia::config_key::initPacketJunkSize).isUndefined()
             && !wgConfig.value(amnezia::config_key::responsePacketJunkSize).isUndefined()
             && !wgConfig.value(amnezia::config_key::initPacketMagicHeader).isUndefined()
             && !wgConfig.value(amnezia::config_key::responsePacketMagicHeader).isUndefined()
             && !wgConfig.value(amnezia::config_key::underloadPacketMagicHeader).isUndefined()
             && !wgConfig.value(amnezia::config_key::transportPacketMagicHeader).isUndefined()) {
    json.insert(amnezia::config_key::junkPacketCount, wgConfig.value(amnezia::config_key::junkPacketCount));
    json.insert(amnezia::config_key::junkPacketMinSize, wgConfig.value(amnezia::config_key::junkPacketMinSize));
    json.insert(amnezia::config_key::junkPacketMaxSize, wgConfig.value(amnezia::config_key::junkPacketMaxSize));
    json.insert(amnezia::config_key::initPacketJunkSize, wgConfig.value(amnezia::config_key::initPacketJunkSize));
    json.insert(amnezia::config_key::responsePacketJunkSize, wgConfig.value(amnezia::config_key::responsePacketJunkSize));
    json.insert(amnezia::config_key::initPacketMagicHeader, wgConfig.value(amnezia::config_key::initPacketMagicHeader));
    json.insert(amnezia::config_key::responsePacketMagicHeader, wgConfig.value(amnezia::config_key::responsePacketMagicHeader));
    json.insert(amnezia::config_key::underloadPacketMagicHeader, wgConfig.value(amnezia::config_key::underloadPacketMagicHeader));
    json.insert(amnezia::config_key::transportPacketMagicHeader, wgConfig.value(amnezia::config_key::transportPacketMagicHeader));
  }

  write(json);
}

void LocalSocketController::deactivate() {
  logger.debug() << "Deactivating";

  if (m_daemonState != eReady) {
    logger.debug() << "No disconnect, controller is not ready";
    emit disconnected();
    return;
  }

  QJsonObject json;
  json.insert("type", "deactivate");
  write(json);
  emit disconnected();
}

void LocalSocketController::checkStatus() {
  logger.debug() << "Check status";

  if (m_daemonState == eReady || m_daemonState == eInitializing) {
    Q_ASSERT(m_socket);

    QJsonObject json;
    json.insert("type", "status");
    write(json);
  }
}

void LocalSocketController::getBackendLogs(
    std::function<void(const QString&)>&& a_callback) {
  logger.debug() << "Backend logs";

  if (m_logCallback) {
    m_logCallback("");
    m_logCallback = nullptr;
  }

  if (m_daemonState != eReady) {
    std::function<void(const QString&)> callback = a_callback;
    callback("");
    return;
  }

  m_logCallback = std::move(a_callback);

  QJsonObject json;
  json.insert("type", "logs");
  write(json);
}

void LocalSocketController::cleanupBackendLogs() {
  logger.debug() << "Cleanup logs";

  if (m_logCallback) {
    m_logCallback("");
    m_logCallback = nullptr;
  }

  if (m_daemonState != eReady) {
    return;
  }

  QJsonObject json;
  json.insert("type", "cleanlogs");
  write(json);
}

void LocalSocketController::readData() {
  logger.debug() << "Reading";

  Q_ASSERT(m_socket);
  Q_ASSERT(m_daemonState == eInitializing || m_daemonState == eReady);
  QByteArray input = m_socket->readAll();
  m_buffer.append(input);

  while (true) {
    int pos = m_buffer.indexOf("\n");
    if (pos == -1) {
      break;
    }

    QByteArray line = m_buffer.left(pos);
    m_buffer.remove(0, pos + 1);

    QByteArray command(line);
    command = command.trimmed();

    if (command.isEmpty()) {
      continue;
    }

    parseCommand(command);
  }
}

void LocalSocketController::parseCommand(const QByteArray& command) {
  QJsonDocument json = QJsonDocument::fromJson(command);
  if (!json.isObject()) {
    logger.error() << "Invalid JSON - object expected";
    return;
  }

  QJsonObject obj = json.object();
  QJsonValue typeValue = obj.value("type");
  if (!typeValue.isString()) {
    logger.error() << "Invalid JSON - no type";
    return;
  }
  QString type = typeValue.toString();

  logger.debug() << "Parse command:" << type;

  if (m_daemonState == eInitializing && type == "status") {
    m_daemonState = eReady;

    QJsonValue connected = obj.value("connected");
    if (!connected.isBool()) {
      logger.error() << "Invalid JSON for status - connected expected";
      return;
    }

    QDateTime datetime;
    if (connected.toBool()) {
      QJsonValue date = obj.value("date");
      if (!date.isString()) {
        logger.error() << "Invalid JSON for status - date expected";
        return;
      }

      datetime = QDateTime::fromString(date.toString());
      if (!datetime.isValid()) {
        logger.error() << "Invalid JSON for status - date is invalid";
        return;
      }
    }

    emit initialized(true, connected.toBool(), datetime);
    return;
  }

  if (m_daemonState != eReady) {
    logger.error() << "Unexpected command";
    return;
  }

  if (type == "status") {
    QJsonValue serverIpv4Gateway = obj.value("serverIpv4Gateway");
    if (!serverIpv4Gateway.isString()) {
      logger.error() << "Unexpected serverIpv4Gateway value";
      return;
    }

    QJsonValue deviceIpv4Address = obj.value("deviceIpv4Address");
    if (!deviceIpv4Address.isString()) {
      logger.error() << "Unexpected deviceIpv4Address value";
      return;
    }

    QJsonValue txBytes = obj.value("txBytes");
    if (!txBytes.isDouble()) {
      logger.error() << "Unexpected txBytes value";
      return;
    }

    QJsonValue rxBytes = obj.value("rxBytes");
    if (!rxBytes.isDouble()) {
      logger.error() << "Unexpected rxBytes value";
      return;
    }

    emit statusUpdated(serverIpv4Gateway.toString(),
                       deviceIpv4Address.toString(), txBytes.toDouble(),
                       rxBytes.toDouble());
    return;
  }

  if (type == "disconnected") {
    disconnectInternal();
    return;
  }

  if (type == "connected") {
    QJsonValue pubkey = obj.value("pubkey");
    if (!pubkey.isString()) {
      logger.error() << "Unexpected pubkey value";
      return;
    }

    logger.debug() << "Handshake completed with:"
                   << pubkey.toString();
    emit connected(pubkey.toString());
    return;
  }

  if (type == "backendFailure") {
    if (!obj.contains("errorCode")) {
      // report a generic error if we dont know what it is.
      logger.error() << "generic backend failure error";
      // REPORTERROR(ErrorHandler::ControllerError, "controller");
      return;
    }
    auto errorCode = static_cast<uint8_t>(obj["errorCode"].toInt());
    if (errorCode >= (uint8_t)DaemonError::DAEMON_ERROR_MAX) {
      // Also report a generic error if the code is invalid.
      logger.error() << "invalid backend failure error code";
      // REPORTERROR(ErrorHandler::ControllerError, "controller");
      return;
    }
    switch (static_cast<DaemonError>(errorCode)) {
      case DaemonError::ERROR_NONE:
        [[fallthrough]];
      case DaemonError::ERROR_FATAL:
        logger.error() << "generic backend failure error (fatal or error none)";
        // REPORTERROR(ErrorHandler::ControllerError, "controller");
        break;
      case DaemonError::ERROR_SPLIT_TUNNEL_INIT_FAILURE:
        [[fallthrough]];
      case DaemonError::ERROR_SPLIT_TUNNEL_START_FAILURE:
        [[fallthrough]];
      case DaemonError::ERROR_SPLIT_TUNNEL_EXCLUDE_FAILURE:
        logger.error() << "split tunnel backend failure error";
        //REPORTERROR(ErrorHandler::SplitTunnelError, "controller");
        break;
      case DaemonError::DAEMON_ERROR_MAX:
        // We should not get here.
        Q_ASSERT(false);
        break;
    }
  }

  if (type == "logs") {
    // We don't care if we are not waiting for logs.
    if (!m_logCallback) {
      return;
    }

    QJsonValue logs = obj.value("logs");
    m_logCallback(logs.isString() ? logs.toString().replace("|", "\n")
                                  : QString());
    m_logCallback = nullptr;
    return;
  }

  logger.warning() << "Invalid command received:" << command;
}

void LocalSocketController::write(const QJsonObject& json) {
  Q_ASSERT(m_socket);
  m_socket->write(QJsonDocument(json).toJson(QJsonDocument::Compact));
  m_socket->write("\n");
  m_socket->flush();
}
