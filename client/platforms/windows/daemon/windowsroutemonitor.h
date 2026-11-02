/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSROUTEMONITOR_H
#define WINDOWSROUTEMONITOR_H

#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2ipdef.h>

#include <QHash>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QTimer>
#include <QVector>

#include "ipaddress.h"

class WindowsRouteMonitor final : public QObject {
  Q_OBJECT

 public:
  WindowsRouteMonitor(quint64 luid, QObject* parent);
  ~WindowsRouteMonitor();

  void setDetaultRouteCapture(bool enable);

  bool addExclusionRoute(const IPAddress& prefix);
  bool deleteExclusionRoute(const IPAddress& prefix);

  int addExclusionRoutes(const QList<IPAddress>& prefixes);
  int deleteExclusionRoutes(const QList<IPAddress>& prefixes);

  void flushExclusionRoutes() { return flushRouteTable(m_exclusionRoutes); };

  quint64 getLuid() const { return m_luid; }

 public slots:
  void routeChanged();
  void scheduleRouteChanged();

 private:
  using RowList = QVector<const MIB_IPFORWARD_ROW2*>;

  struct NextHop {
    quint64 luid = 0;
    SOCKADDR_INET address = {};
  };

  bool isRouteExcluded(const IP_ADDRESS_PREFIX* dest) const;
  static bool routeContainsDest(const IP_ADDRESS_PREFIX* route,
                                const IP_ADDRESS_PREFIX* dest);
  static QHostAddress prefixToAddress(const IP_ADDRESS_PREFIX* dest);

  static MIB_IPFORWARD_ROW2* buildExclusionRow(const IPAddress& prefix);
  RowList exclusionCandidates(void* table, int family) const;
  NextHop resolveExclusionHop(const MIB_IPFORWARD_ROW2* data,
                              const RowList& candidates) const;
  static bool hopChanged(const MIB_IPFORWARD_ROW2* data, const NextHop& hop);
  void applyExclusionHop(MIB_IPFORWARD_ROW2* data, const NextHop& hop);
  void removeRoute(MIB_IPFORWARD_ROW2* data, const IPAddress& prefix);

  void flushRouteTable(QHash<IPAddress, MIB_IPFORWARD_ROW2*>& table);
  void updateExclusionRoute(MIB_IPFORWARD_ROW2* data, void* table);
  void updateInterfaceMetrics(int family);
  void updateCapturedRoutes(int family);
  void updateCapturedRoutes(int family, void* table);

  QHash<IPAddress, MIB_IPFORWARD_ROW2*> m_exclusionRoutes;
  QMap<quint64, ULONG> m_interfaceMetricsIpv4;
  QMap<quint64, ULONG> m_interfaceMetricsIpv6;

  // Default route cloning
  bool m_defaultRouteCapture = false;
  QHash<IPAddress, MIB_IPFORWARD_ROW2*> m_clonedRoutes;
  // Clones refused with ERROR_OBJECT_ALREADY_EXISTS, not retried.
  QSet<IPAddress> m_captureRefused;

  const quint64 m_luid = 0;
  HANDLE m_routeHandle = INVALID_HANDLE_VALUE;

  QTimer m_routeChangedDebounce;
};

#endif /* WINDOWSROUTEMONITOR_H */
