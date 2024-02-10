/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "windowsutils.h"

#include <Windows.h>
#include <errhandlingapi.h>

#include <QSettings>
#include <QSysInfo>

#include "logger.h"

namespace {
Logger logger("WindowsUtils");
}  // namespace

constexpr const int WINDOWS_11_BUILD =
    22000;  // Build Number of the first release win 11 iso

QString WindowsUtils::getErrorMessage(quint32 code) {
  LPSTR messageBuffer = nullptr;
  size_t size = FormatMessageA(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
      nullptr, code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPSTR)&messageBuffer, 0, nullptr);

  std::string message(messageBuffer, size);
  QString result(message.c_str());
  LocalFree(messageBuffer);
  return result;
}

QString WindowsUtils::getErrorMessage() {
  return getErrorMessage(GetLastError());
}

// A simple function to log windows error messages.
void WindowsUtils::windowsLog(const QString& msg) {
  QString errmsg = getErrorMessage();
  logger.error() << msg << "-" << errmsg;
}

// Static
QString WindowsUtils::windowsVersion() {
  QSettings regCurrentVersion(
      "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
      QSettings::NativeFormat);

  int buildNr = regCurrentVersion.value("CurrentBuild").toInt();
  if (buildNr >= WINDOWS_11_BUILD) {
    return "11";
  }
  return QSysInfo::productVersion();
}

// static
void WindowsUtils::forceCrash() {
  RaiseException(0x0000DEAD, EXCEPTION_NONCONTINUABLE, 0, NULL);
}
