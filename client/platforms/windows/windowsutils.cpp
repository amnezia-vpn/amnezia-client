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

constexpr const wchar_t kThemeWatcherClassName[] = L"AmneziaVpnThemeWatcher";

struct ThemeObserverState
{
    std::function<void()> callback;
    HWND hwnd = nullptr;
};

ThemeObserverState g_themeObserver;

bool registryUsesDarkTheme(const QSettings &settings, const QString &lightThemeKey)
{
    if (settings.contains(lightThemeKey)) {
        return settings.value(lightThemeKey).toInt() != 1;
    }
    return false;
}

LRESULT CALLBACK themeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Q_UNUSED(wParam);

    if (msg == WM_SETTINGCHANGE && lParam != 0) {
        const wchar_t *section = reinterpret_cast<const wchar_t *>(lParam);
        if (wcscmp(section, L"ImmersiveColorSet") == 0 || wcscmp(section, L"WindowsThemeElement") == 0) {
            if (g_themeObserver.callback) {
                g_themeObserver.callback();
            }
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
} // namespace

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

// static
bool WindowsUtils::isDarkTheme() {
  QSettings settings(
          QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
          QSettings::NativeFormat);

  if (settings.contains(QStringLiteral("SystemUsesLightTheme"))) {
      return registryUsesDarkTheme(settings, QStringLiteral("SystemUsesLightTheme"));
  }

  if (settings.contains(QStringLiteral("AppsUseLightTheme"))) {
      return registryUsesDarkTheme(settings, QStringLiteral("AppsUseLightTheme"));
  }

  logger.warning() << "SystemUsesLightTheme registry key is unavailable; assuming dark theme";
  return true;
}

void WindowsUtils::installThemeChangeObserver(std::function<void()> callback)
{
    g_themeObserver.callback = std::move(callback);

    if (g_themeObserver.hwnd) {
        return;
    }

    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW wc = {};
    wc.lpfnWndProc = themeWndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kThemeWatcherClassName;

    WNDCLASSW existing = {};
    if (!GetClassInfoW(instance, kThemeWatcherClassName, &existing)) {
        if (!RegisterClassW(&wc)) {
            WindowsUtils::windowsLog("Failed to register theme watcher window class");
            return;
        }
    }

    g_themeObserver.hwnd = CreateWindowExW(0, kThemeWatcherClassName, L"AmneziaVpnThemeWatcher", 0, 0, 0, 0, 0,
                                           HWND_MESSAGE, nullptr, instance, nullptr);
    if (!g_themeObserver.hwnd) {
        WindowsUtils::windowsLog("Failed to create theme watcher window");
    }
}
