/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef CONTROLLERIMPL_H
#define CONTROLLERIMPL_H

#include <QDateTime>
#include <QObject>
#include <functional>

class Keys;
class Device;
class Server;
class QDateTime;
class IPAddress;
class QHostAddress;

// This object is allocated when the VPN is about to be activated.
// It's kept alive, basically forever, except in these scenarios, in which it's
// recreated:
// - the user does a logout
// - there is an authentication falure
class ControllerImpl : public QObject {
  Q_OBJECT

 public:
  ControllerImpl() = default;

  virtual ~ControllerImpl() = default;

  // This method is called to initialize the controller. The initialization
  // is completed when the signal "initialized" is emitted.
  virtual void initialize(const Device* device, const Keys* keys) = 0;

  // This method is called when the VPN client needs to activate the VPN
  // tunnel. It's called only at the end of the initialization process.  When
  // this method is called, the VPN client is in "connecting" state.  This
  // state terminates when the "connected" (or the "disconnected") signal is
  // received.
  virtual void activate(const QJsonObject& config) = 0;

  // This method terminates the VPN tunnel. The VPN client is in
  // "disconnecting" state until the "disconnected" signal is received.
  virtual void deactivate() = 0;

  // This method is used to retrieve the VPN tunnel status (mainly the number
  // of bytes sent and received). It's called always when the VPN tunnel is
  // active.
  virtual void checkStatus() = 0;

  // This method is used to retrieve the logs from the backend service. Use
  // the callback to report logs when available.
  virtual void getBackendLogs(
      std::function<void(const QString& logs)>&& callback) = 0;

  // Cleanup the backend logs.
  virtual void cleanupBackendLogs() = 0;

  // Whether the controller supports multihop
  virtual bool multihopSupported() { return false; }

  virtual bool silentServerSwitchingSupported() const { return true; }

 signals:
  // This signal is emitted when the controller is initialized. Note that the
  // VPN tunnel can be already active. In this case, "connected" should be set
  // to true and the "connectionDate" should be set to the activation date if
  // known.
  // If "status" is set to false, the backend service is considered unavailable.
  void initialized(bool status, bool connected,
                   const QDateTime& connectionDate);

  // These 2 signals can be dispatched at any time.
  void connected(const QString& pubkey,
                 const QDateTime& connectionTimestamp = QDateTime());
  void disconnected();

  // This method should be emitted after a checkStatus() call.
  // "serverIpv4Gateway" is the current VPN tunnel gateway.
  // "deviceIpv4Address" is the address of the VPN client.
  // "txBytes" and "rxBytes" contain the number of transmitted and received
  // bytes since the last statusUpdated signal.
  void statusUpdated(const QString& serverIpv4Gateway,
                     const QString& deviceIpv4Address, uint64_t txBytes,
                     uint64_t rxBytes);
};

#endif  // CONTROLLERIMPL_H
