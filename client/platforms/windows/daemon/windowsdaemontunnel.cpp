/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsdaemontunnel.h"

#include <Windows.h>

#include <QCoreApplication>

//#include "commandlineparser.h"
#include "constants.h"
#include "leakdetector.h"
#include "logger.h"
#include "platforms/windows/daemon/wireguardutilswindows.h"
#include "platforms/windows/windowsutils.h"

namespace {
Logger logger("WindowsDaemonTunnel");
}  // namespace

WindowsDaemonTunnel::WindowsDaemonTunnel() {
  MZ_COUNT_CTOR(WindowsDaemonTunnel);
}

WindowsDaemonTunnel::~WindowsDaemonTunnel() {
  MZ_COUNT_DTOR(WindowsDaemonTunnel);
}

int WindowsDaemonTunnel::run(QStringList& tokens) {
  Q_ASSERT(!tokens.isEmpty());

  logger.debug() << "Tunnel daemon service is starting";

  QCoreApplication app();

  QCoreApplication::setApplicationName("Amnezia VPN Tunnel");
  QCoreApplication::setApplicationVersion(Constants::versionString());

  if (tokens.length() != 2) {
    logger.error() << "Expected 1 parameter only: the config file.";
    return 1;
  }
  QString maybeConfig = tokens.at(1);

  if (!maybeConfig.startsWith("[Interface]")) {
    logger.error() << "parameter Does not seem to be a config";
    return 1;
  }
  // This process will be used by the wireguard tunnel. No need to call
  // FreeLibrary.
  HMODULE tunnelLib = LoadLibrary(TEXT("tunnel.dll"));
  if (!tunnelLib) {
    WindowsUtils::windowsLog("Failed to load tunnel.dll");
    return 1;
  }

  typedef bool WireGuardTunnelService(const ushort* settings,
                                      const ushort* name);

  WireGuardTunnelService* tunnelProc = (WireGuardTunnelService*)GetProcAddress(
      tunnelLib, "WireGuardTunnelService");
  if (!tunnelProc) {
    WindowsUtils::windowsLog("Failed to get WireGuardTunnelService function");
    return 1;
  }
  auto name = WireguardUtilsWindows::s_interfaceName();
  if (!tunnelProc(maybeConfig.utf16(), name.utf16())) {
    logger.error() << "Failed to activate the tunnel service";
    return 1;
  }

  return 0;
}

//static Command::RegistrationProxy<WindowsDaemonTunnel>
//    s_commandWindowsDaemonTunnel;
