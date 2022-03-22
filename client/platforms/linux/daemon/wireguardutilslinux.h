/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WIREGUARDUTILSLINUX_H
#define WIREGUARDUTILSLINUX_H

#include "daemon/wireguardutils.h"
#include <QObject>
#include <QSocketNotifier>
#include <QStringList>

class WireguardUtilsLinux final : public WireguardUtils {
  Q_OBJECT

 public:
  WireguardUtilsLinux(QObject* parent);
  ~WireguardUtilsLinux();
  bool interfaceExists() override;
  bool addInterface(const InterfaceConfig& config) override;
  bool deleteInterface() override;

  bool updatePeer(const InterfaceConfig& config) override;
  bool deletePeer(const QString& pubkey) override;
  peerStatus getPeerStatus(const QString& pubkey) override;

  bool updateRoutePrefix(const IPAddressRange& prefix, int hopindex) override;
  bool deleteRoutePrefix(const IPAddressRange& prefix, int hopindex) override;

  QString getDefaultCgroup() const { return m_cgroups; }
  QString getExcludeCgroup() const;
  QString getBlockCgroup() const;

 private:
  QStringList currentInterfaces();
  bool setPeerEndpoint(struct sockaddr* sa, const QString& address, int port);
  bool addPeerPrefix(struct wg_peer* peer, const IPAddressRange& prefix);
  bool rtmSendRule(int action, int flags, int addrfamily);
  bool rtmSendRoute(int action, int flags, const IPAddressRange& prefix,
                    int hopindex);
  static bool setupCgroupClass(const QString& path, unsigned long classid);
  static bool buildAllowedIp(struct wg_allowedip*,
                             const IPAddressRange& prefix);

  static QString printablePubkey(const QString& pubkey);

  int m_nlsock = -1;
  int m_nlseq = 0;
  QSocketNotifier* m_notifier = nullptr;
  QString m_cgroups;

 private slots:
  void nlsockReady();
};

#endif  // WIREGUARDUTILSLINUX_H
