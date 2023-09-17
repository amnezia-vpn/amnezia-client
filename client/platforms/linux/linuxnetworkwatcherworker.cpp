/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "linuxnetworkwatcherworker.h"

#include <QtDBus/QtDBus>

#include "leakdetector.h"
#include "logger.h"

// https://developer.gnome.org/NetworkManager/stable/nm-dbus-types.html#NMDeviceType
#ifndef NM_DEVICE_TYPE_WIFI
#  define NM_DEVICE_TYPE_WIFI 2
#endif

// https://developer.gnome.org/NetworkManager/stable/nm-dbus-types.html#NM80211ApFlags
// Wifi network has no security
#ifndef NM_802_11_AP_SEC_NONE
#  define NM_802_11_AP_SEC_NONE 0x00000000
#endif

// Wifi network has WEP (40 bits)
#ifndef NM_802_11_AP_SEC_PAIR_WEP40
#  define NM_802_11_AP_SEC_PAIR_WEP40 0x00000001
#endif

// Wifi network has WEP (104 bits)
#ifndef NM_802_11_AP_SEC_PAIR_WEP104
#  define NM_802_11_AP_SEC_PAIR_WEP104 0x00000002
#endif

#define NM_802_11_AP_SEC_WEAK_CRYPTO \
  (NM_802_11_AP_SEC_PAIR_WEP40 | NM_802_11_AP_SEC_PAIR_WEP104)

constexpr const char* DBUS_NETWORKMANAGER = "org.freedesktop.NetworkManager";

namespace {
Logger logger("LinuxNetworkWatcherWorker");
}

static inline bool checkUnsecureFlags(int rsnFlags, int wpaFlags) {
  // If neither WPA nor WPA2/RSN are supported, then the network is unencrypted
  if (rsnFlags == NM_802_11_AP_SEC_NONE && wpaFlags == NM_802_11_AP_SEC_NONE) {
    return false;
  }

  // Consider the user of weak cryptography to be unsecure
  if ((rsnFlags & NM_802_11_AP_SEC_WEAK_CRYPTO) ||
      (wpaFlags & NM_802_11_AP_SEC_WEAK_CRYPTO)) {
    return false;
  }
  // Otherwise, the network is secured with reasonable cryptography
  return true;
}

LinuxNetworkWatcherWorker::LinuxNetworkWatcherWorker(QThread* thread) {
  MZ_COUNT_CTOR(LinuxNetworkWatcherWorker);
  moveToThread(thread);
}

LinuxNetworkWatcherWorker::~LinuxNetworkWatcherWorker() {
  MZ_COUNT_DTOR(LinuxNetworkWatcherWorker);
}

void LinuxNetworkWatcherWorker::initialize() {
  logger.debug() << "initialize";

  logger.debug()
      << "Retrieving the list of wifi network devices from NetworkManager";

  // To know the NeworkManager DBus methods and properties, read the official
  // documentation:
  // https://developer.gnome.org/NetworkManager/stable/gdbus-org.freedesktop.NetworkManager.html

  QDBusInterface nm(DBUS_NETWORKMANAGER, "/org/freedesktop/NetworkManager",
                    DBUS_NETWORKMANAGER, QDBusConnection::systemBus());
  if (!nm.isValid()) {
    logger.error()
        << "Failed to connect to the network manager via system dbus";
    return;
  }

  QDBusMessage msg = nm.call("GetDevices");
  QDBusArgument arg = msg.arguments().at(0).value<QDBusArgument>();
  if (arg.currentType() != QDBusArgument::ArrayType) {
    logger.error() << "Expected an array of devices";
    return;
  }

  QList<QDBusObjectPath> paths = qdbus_cast<QList<QDBusObjectPath> >(arg);
  for (const QDBusObjectPath& path : paths) {
    QString devicePath = path.path();
    QDBusInterface device(DBUS_NETWORKMANAGER, devicePath,
                          "org.freedesktop.NetworkManager.Device",
                          QDBusConnection::systemBus());
    if (device.property("DeviceType").toInt() != NM_DEVICE_TYPE_WIFI) {
      continue;
    }

    logger.debug() << "Found a wifi device:" << devicePath;
    m_devicePaths.append(devicePath);

    // Here we monitor the changes.
    QDBusConnection::systemBus().connect(
        DBUS_NETWORKMANAGER, devicePath, "org.freedesktop.DBus.Properties",
        "PropertiesChanged", this,
        SLOT(propertyChanged(QString, QVariantMap, QStringList)));
  }

  if (m_devicePaths.isEmpty()) {
    logger.warning() << "No wifi devices found";
    return;
  }

  // We could be already be activated.
  checkDevices();
}

void LinuxNetworkWatcherWorker::propertyChanged(QString interface,
                                                QVariantMap properties,
                                                QStringList list) {
  Q_UNUSED(list);

  logger.debug() << "Properties changed for interface" << interface;

  if (!properties.contains("ActiveAccessPoint")) {
    logger.debug() << "Access point did not changed. Ignoring the changes";
    return;
  }

  checkDevices();
}

void LinuxNetworkWatcherWorker::checkDevices() {
  logger.debug() << "Checking devices";

  for (const QString& devicePath : m_devicePaths) {
    QDBusInterface wifiDevice(DBUS_NETWORKMANAGER, devicePath,
                              "org.freedesktop.NetworkManager.Device.Wireless",
                              QDBusConnection::systemBus());

    // Check the access point path
    QString accessPointPath = wifiDevice.property("ActiveAccessPoint")
                                  .value<QDBusObjectPath>()
                                  .path();
    if (accessPointPath.isEmpty()) {
      logger.warning() << "No access point found";
      continue;
    }

    QDBusInterface ap(DBUS_NETWORKMANAGER, accessPointPath,
                      "org.freedesktop.NetworkManager.AccessPoint",
                      QDBusConnection::systemBus());

    QVariant rsnFlags = ap.property("RsnFlags");
    QVariant wpaFlags = ap.property("WpaFlags");
    if (!rsnFlags.isValid() || !wpaFlags.isValid()) {
      // We are probably not connected.
      continue;
    }

    if (!checkUnsecureFlags(rsnFlags.toInt(), wpaFlags.toInt())) {
      QString ssid = ap.property("Ssid").toString();
      QString bssid = ap.property("HwAddress").toString();

      // We have found 1 unsecured network. We don't need to check other wifi
      // network devices.
      logger.warning() << "Unsecured AP detected!"
                       << "rsnFlags:" << rsnFlags.toInt()
                       << "wpaFlags:" << wpaFlags.toInt()
                       << "ssid:" << logger.sensitive(ssid);
      emit unsecuredNetwork(ssid, bssid);
      break;
    }
  }
}
