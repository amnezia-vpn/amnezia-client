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

#include <QObject>

#include "ipaddress.h"

class WindowsRouteMonitor final : public QObject {
  Q_OBJECT

 public:
  WindowsRouteMonitor(QObject* parent);
  ~WindowsRouteMonitor();

  bool addExclusionRoute(const IPAddress& prefix);
  bool deleteExclusionRoute(const IPAddress& prefix);
  void flushExclusionRoutes();

  void setLuid(quint64 luid) { m_luid = luid; }
  quint64 getLuid() { return m_luid; }

 public slots:
  void routeChanged();

 private:
  void updateExclusionRoute(MIB_IPFORWARD_ROW2* data, void* table);
  void updateValidInterfaces(int family);

  QHash<IPAddress, MIB_IPFORWARD_ROW2*> m_exclusionRoutes;
  QList<quint64> m_validInterfacesIpv4;
  QList<quint64> m_validInterfacesIpv6;

  quint64 m_luid = 0;
  HANDLE m_routeHandle = INVALID_HANDLE_VALUE;
};

#endif /* WINDOWSROUTEMONITOR_H */
