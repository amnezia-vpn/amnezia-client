/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosnetworkwatcher.h"
#include "leakdetector.h"
#include "logger.h"

#import <CoreWLAN/CoreWLAN.h>

namespace {
Logger logger(LOG_MACOS, "MacOSNetworkWatcher");
}

@interface MacOSNetworkWatcherDelegate : NSObject <CWEventDelegate> {
  MacOSNetworkWatcher* m_watcher;
}
@end

@implementation MacOSNetworkWatcherDelegate

- (id)initWithObject:(MacOSNetworkWatcher*)watcher {
  self = [super init];
  if (self) {
    m_watcher = watcher;
  }
  return self;
}

- (void)bssidDidChangeForWiFiInterfaceWithName:(NSString*)interfaceName {
  logger.debug() << "BSSID changed!" << QString::fromNSString(interfaceName);

  if (m_watcher) {
    m_watcher->checkInterface();
  }
}

@end

MacOSNetworkWatcher::MacOSNetworkWatcher(QObject* parent) : NetworkWatcherImpl(parent) {
  MVPN_COUNT_CTOR(MacOSNetworkWatcher);
}

MacOSNetworkWatcher::~MacOSNetworkWatcher() {
  MVPN_COUNT_DTOR(MacOSNetworkWatcher);

  if (m_delegate) {
    CWWiFiClient* client = CWWiFiClient.sharedWiFiClient;
    if (!client) {
      logger.debug() << "Unable to retrieve the CWWiFiClient shared instance";
      return;
    }

    [client stopMonitoringAllEventsAndReturnError:nullptr];
    [static_cast<MacOSNetworkWatcherDelegate*>(m_delegate) dealloc];
    m_delegate = nullptr;
  }
}

void MacOSNetworkWatcher::initialize() {
  // Nothing to do here
}

void MacOSNetworkWatcher::start() {
  NetworkWatcherImpl::start();

  checkInterface();

  if (m_delegate) {
    logger.debug() << "Delegate already registered";
    return;
  }

  CWWiFiClient* client = CWWiFiClient.sharedWiFiClient;
  if (!client) {
    logger.error() << "Unable to retrieve the CWWiFiClient shared instance";
    return;
  }

  logger.debug() << "Registering delegate";
  m_delegate = [[MacOSNetworkWatcherDelegate alloc] initWithObject:this];
  [client setDelegate:static_cast<MacOSNetworkWatcherDelegate*>(m_delegate)];
  [client startMonitoringEventWithType:CWEventTypeBSSIDDidChange error:nullptr];
}

void MacOSNetworkWatcher::checkInterface() {
  logger.debug() << "Checking interface";

  if (!isActive()) {
    logger.debug() << "Feature disabled";
    return;
  }

  CWWiFiClient* client = CWWiFiClient.sharedWiFiClient;
  if (!client) {
    logger.debug() << "Unable to retrieve the CWWiFiClient shared instance";
    return;
  }

  CWInterface* interface = [client interface];
  if (!interface) {
    logger.debug() << "No default wifi interface";
    return;
  }

  if (![interface powerOn]) {
    logger.debug() << "The interface is off";
    return;
  }

  NSString* ssidNS = [interface ssid];
  if (!ssidNS) {
    logger.debug() << "WiFi is not in used";
    return;
  }

  QString ssid = QString::fromNSString(ssidNS);
  if (ssid.isEmpty()) {
    logger.debug() << "WiFi doesn't have a valid SSID";
    return;
  }

  CWSecurity security = [interface security];
  if (security == kCWSecurityNone || security == kCWSecurityWEP) {
    logger.debug() << "Unsecured network found!";
    emit unsecuredNetwork(ssid, ssid);
    return;
  }

  logger.debug() << "Secure WiFi interface";
}
