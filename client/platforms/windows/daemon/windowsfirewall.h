/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSFIREWALL_H
#define WINDOWSFIREWALL_H

#pragma comment(lib, "Fwpuclnt")

// Note: The windows.h import needs to come before the fwpmu.h import.
// clang-format off
#include <windows.h>
#include <fwpmu.h>
// clang-format on

#include <QByteArray>
#include <QHostAddress>
#include <QMap>
#include <QObject>
#include <QString>

#include "../client/daemon/interfaceconfig.h"

class IpAdressRange;
struct FWP_VALUE0_;
struct FWP_CONDITION_VALUE0_;

class WindowsFirewall final : public QObject {
 public:
  /**
   * @brief Opens the Windows Filtering Platform, initializes the session,
   * sublayer. Returns a WindowsFirewall object if successful, otherwise
   * nullptr. If there is already a WindowsFirewall object, it will be returned.
   *
   * @param parent - parent QObject
   * @return WindowsFirewall* - nullptr if failed to open the Windows Filtering
   * Platform.
   */
  static WindowsFirewall* create(QObject* parent);
  ~WindowsFirewall() override;

  bool enableInterface(int vpnAdapterIndex, const QString& ifname = QString());
  bool enableLanBypass(const QList<IPAddress>& ranges);
  bool enablePeerTraffic(const InterfaceConfig& config);
  bool disableKillSwitch();
  bool disableKillSwitchForTunnel(const QString& ifname);
  bool allowAllTraffic();
  bool allowTrafficRange(const QStringList& ranges, const QString& ifname = QString());

 private:
  static bool initSublayer();
  WindowsFirewall(HANDLE session, QObject* parent);
  HANDLE m_sessionHandle;
  bool m_init = false;
  QList<uint64_t> m_globalRules;
  QMap<QString, QList<uint64_t>> m_tunnelRules;
  bool m_baseRulesInstalled = false;
  bool m_blockAllInstalled = false;

  bool allowTrafficForAppOnAll(const QString& exePath, int weight,
                               const QString& title, QList<uint64_t>& target);
  bool blockTrafficTo(const QList<IPAddress>& range, uint8_t weight,
                      const QString& title, QList<uint64_t>& target);
  bool blockTrafficTo(const IPAddress& addr, uint8_t weight,
                      const QString& title, QList<uint64_t>& target);
  bool blockTrafficOnPort(uint port, uint8_t weight, const QString& title,
                          QList<uint64_t>& target);
  bool allowTrafficTo(const IPAddress& addr, int weight, const QString& title,
                      QList<uint64_t>& target);
  bool allowTrafficTo(const QHostAddress& targetIP, uint port, int weight,
                      const QString& title, QList<uint64_t>& target);
  bool allowTrafficOfAdapter(int networkAdapter, uint8_t weight,
                             const QString& title, QList<uint64_t>& target);
  bool allowDHCPTraffic(uint8_t weight, const QString& title,
                        QList<uint64_t>& target);
  bool allowHyperVTraffic(uint8_t weight, const QString& title,
                          QList<uint64_t>& target);
  bool allowLoopbackTraffic(uint8_t weight, const QString& title,
                            QList<uint64_t>& target);

  // Utils
  QString getCurrentPath();
  void importAddress(const QHostAddress& addr, OUT FWP_VALUE0_& value,
                     OUT QByteArray* v6DataBuffer);
  void importAddress(const QHostAddress& addr, OUT FWP_CONDITION_VALUE0_& value,
                     OUT QByteArray* v6DataBuffer);
  bool enableFilter(FWPM_FILTER0* filter, const QString& title,
                    const QString& description, QList<uint64_t>& target);
};

#endif  // WINDOWSFIREWALL_H
