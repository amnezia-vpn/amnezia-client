/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSFIREWALL_H
#define WINDOWSFIREWALL_H

#pragma comment(lib, "Fwpuclnt")

//#include "../../daemon/interfaceconfig.h"

#include "ipaddress.h"

#include <windows.h>
#include <fwpmu.h>
#include <QString>
#include <QObject>
#include <QHostAddress>
#include <QByteArray>
class IpAdressRange;
struct FWP_VALUE0_;
struct FWP_CONDITION_VALUE0_;

class WindowsFirewall final : public QObject {
 public:
  ~WindowsFirewall();

  static WindowsFirewall* instance();
  bool init();

  bool enableKillSwitch(int vpnAdapterIndex);
  //bool enablePeerTraffic(const InterfaceConfig& config);
  bool disablePeerTraffic(const QString& pubkey);
  bool disableKillSwitch();

 private:
  WindowsFirewall(QObject* parent);
  HANDLE m_sessionHandle{INVALID_HANDLE_VALUE};
  bool m_init = false;
  QList<uint64_t> m_activeRules;
  QMultiMap<QString, uint64_t> m_peerRules;

  bool allowTrafficForAppOnAll(const QString& exePath, int weight,
                               const QString& title);
  bool blockTrafficTo(const QList<IPAddress>& range, uint8_t weight,
                      const QString& title, const QString& peer = QString());
  bool blockTrafficTo(const IPAddress& addr, uint8_t weight,
                      const QString& title, const QString& peer = QString());
  bool blockTrafficOnPort(uint port, uint8_t weight, const QString& title);
  bool allowTrafficTo(const QHostAddress& targetIP, uint port, int weight,
                      const QString& title, const QString& peer = QString());
  bool allowTrafficOfAdapter(int networkAdapter, uint8_t weight,
                             const QString& title);
  bool allowDHCPTraffic(uint8_t weight, const QString& title);
  bool allowHyperVTraffic(uint8_t weight, const QString& title);
  bool allowLoopbackTraffic(uint8_t weight, const QString& title);

  // Utils
  QString getCurrentPath();
  void importAddress(const QHostAddress& addr, OUT FWP_VALUE0_& value,
                     OUT QByteArray* v6DataBuffer);
  void importAddress(const QHostAddress& addr, OUT FWP_CONDITION_VALUE0_& value,
                     OUT QByteArray* v6DataBuffer);
  bool enableFilter(FWPM_FILTER0* filter, const QString& title,
                    const QString& description,
                    const QString& peer = QString());
};

#endif  // WINDOWSFIREWALL_H
