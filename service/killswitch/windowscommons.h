/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef WINDOWSCOMMONS_H
#define WINDOWSCOMMONS_H

#include <QString>

constexpr char IKEV2[] {"AmneziaVPN IKEv2\0"};

class QHostAddress;

class WindowsCommons final {
 public:
  static QString getErrorMessage();
  static void windowsLog(const QString& msg);

  static QString tunnelConfigFile();
  static QString tunnelLogFile();

  // Returns the Interface Index of the VPN Adapter
  static int VPNAdapterIndex();
  // Returns the Interface Index that could Route to dst
  static int AdapterIndexTo(const QHostAddress& dst);
  // Returns the Path of the Current process
  static QString getCurrentPath();
  // Returns the major version of Windows
  static QString WindowsVersion();
};

#endif  // WINDOWSCOMMONS_H
