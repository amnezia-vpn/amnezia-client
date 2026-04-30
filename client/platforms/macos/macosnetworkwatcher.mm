/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosnetworkwatcher.h"
#include "leakdetector.h"
#include "logger.h"

#include <QProcess>
#include <QMetaObject>
#include <pthread.h>
#include <iostream>

#import <CoreWLAN/CoreWLAN.h>
#import <Network/Network.h>

namespace {
Logger logger("MacOSNetworkWatcher");
}

// Global variables for CFRunLoop thread
static pthread_t g_powerThread;
static CFRunLoopRef g_powerRunLoop = nullptr;
static bool g_shouldStopPowerThread = false;
static PowerNotificationsListener* g_powerListener = nullptr;

// Thread function for dedicated CFRunLoop
void* powerMonitoringThread(void* arg) {
    logger.debug() << "Power monitoring thread started";
    
    PowerNotificationsListener* listener = static_cast<PowerNotificationsListener*>(arg);
    
    // Get the runloop for this thread
    g_powerRunLoop = CFRunLoopGetCurrent();
    
    // Register for power notifications in this thread
    listener->registerForNotifications();
    
    // Run the CFRunLoop - this will block until CFRunLoopStop is called
    while (!g_shouldStopPowerThread) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 1.0, true);
    }
    
    // Cleanup
    listener->cleanup();
    g_powerRunLoop = nullptr;
    
    logger.debug() << "Power monitoring thread finished";
    return nullptr;
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
    // Emit networkChanged signal when BSSID changes
    emit m_watcher->networkChanged(QString::fromNSString(interfaceName));
  }
}

@end

void PowerNotificationsListener::registerForNotifications()
{
    logger.debug() << "Registering for system power notifications in dedicated thread";
    
    rootPowerDomain = IORegisterForSystemPower(this, &notifyPortRef, sleepWakeupCallBack, &notifierObj);
    if (rootPowerDomain == IO_OBJECT_NULL) {
        logger.error() << "Failed to register for system power notifications!";
        return;
    }

    // Add the notification port to the current runloop (dedicated thread)
    CFRunLoopAddSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(notifyPortRef), kCFRunLoopCommonModes);
    logger.debug() << "Power notifications registered successfully";
}

void PowerNotificationsListener::cleanup()
{
    if (notifyPortRef != nullptr) {
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), IONotificationPortGetRunLoopSource(notifyPortRef), kCFRunLoopCommonModes);
        IONotificationPortDestroy(notifyPortRef);
        notifyPortRef = nullptr;
    }
    
    if (notifierObj != IO_OBJECT_NULL) {
        IODeregisterForSystemPower(&notifierObj);
        notifierObj = IO_OBJECT_NULL;
    }
    
    if (rootPowerDomain != IO_OBJECT_NULL) {
        IOServiceClose(rootPowerDomain);
        rootPowerDomain = IO_OBJECT_NULL;
    }
}

void PowerNotificationsListener::sleepWakeupCallBack(void *refParam, io_service_t service, natural_t messageType, void *messageArgument)
{
	Q_UNUSED(service)

    auto listener = static_cast<PowerNotificationsListener *>(refParam);

    logger.debug() << "Power callback received, messageType:" << messageType;
    switch (messageType) {
    case kIOMessageCanSystemSleep:
        /* Idle sleep is about to kick in. This message will not be sent for forced sleep.
         * Applications have a chance to prevent sleep by calling IOCancelPowerChange.
         * Most applications should not prevent idle sleep. Power Management waits up to
         * 30 seconds for you to either allow or deny idle sleep. If you don’t acknowledge
         * this power change by calling either IOAllowPowerChange or IOCancelPowerChange,
         * the system will wait 30 seconds then go to sleep.
         */

        logger.debug() << "System power message: can system sleep?";

        // Uncomment to cancel idle sleep
        // IOCancelPowerChange(thiz->rootPowerDomain, reinterpret_cast<long>(messageArgument));

        // Allow idle sleep
        IOAllowPowerChange(listener->rootPowerDomain, reinterpret_cast<long>(messageArgument));
        break;

    case kIOMessageSystemWillNotSleep:
        /* Announces that the system has retracted a previous attempt to sleep; it
         * follows `kIOMessageCanSystemSleep`.
         */
        logger.debug() << "System power message: system will NOT sleep.";
        break;

    case kIOMessageSystemWillSleep:
        /* The system WILL go to sleep. If you do not call IOAllowPowerChange or
         * IOCancelPowerChange to acknowledge this message, sleep will be delayed by
         * 30 seconds.
         *
         * NOTE: If you call IOCancelPowerChange to deny sleep it returns kIOReturnSuccess,
         * however the system WILL still go to sleep.
         */

        logger.debug() << "System power message: system WILL sleep";
        IOAllowPowerChange(listener->rootPowerDomain, reinterpret_cast<long>(messageArgument));
        break;

    case kIOMessageSystemWillPowerOn:
        /* Announces that the system is beginning to power the device tree; most devices
         * are still unavailable at this point.
         */
        /* From the documentation:
         *
         * - kIOMessageSystemWillPowerOn is delivered at early wakeup time, before most hardware
         * has been powered on. Be aware that any attempts to access disk, network, the display,
         * etc. may result in errors or blocking your process until those resources become
         * available.
         *
         * So we do NOT log this event.
         */
        break;

    case kIOMessageSystemHasPoweredOn:
        /* Announces that the system and its devices have woken up. */
        logger.debug() << "System has powered on - emitting wakeup signal from dedicated CFRunLoop thread";
        if (listener->m_watcher) {
            // Use QMetaObject::invokeMethod for thread-safe signal emission
            QMetaObject::invokeMethod(listener->m_watcher, "wakeup", Qt::QueuedConnection);
        }
        break;

    default:
        logger.debug() << "System power message: other event: " << messageType;
        /* Not a system sleep and wake notification. */
        break;
    }
}

MacOSNetworkWatcher::MacOSNetworkWatcher(QObject* parent) : IOSNetworkWatcher(parent), m_powerlistener(this) {
  MZ_COUNT_CTOR(MacOSNetworkWatcher);
}

MacOSNetworkWatcher::~MacOSNetworkWatcher() {
  MZ_COUNT_DTOR(MacOSNetworkWatcher);
  
  // Stop the dedicated power monitoring thread
  if (g_powerListener) {
    logger.debug() << "Stopping dedicated power monitoring thread";
    g_shouldStopPowerThread = true;
    
    if (g_powerRunLoop) {
      CFRunLoopStop(g_powerRunLoop);
    }
    
    // Wait for thread to finish
    pthread_join(g_powerThread, nullptr);
    g_powerListener = nullptr;
  }
  
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

void MacOSNetworkWatcher::start() {
  NetworkWatcherImpl::start();

  checkInterface();

  if (m_delegate) {
    logger.debug() << "Delegate already registered";
    return;
  }
  
  // Start dedicated power monitoring thread with CFRunLoop
  if (!g_powerListener) {
    g_powerListener = &m_powerlistener;
    g_shouldStopPowerThread = false;
    
    int result = pthread_create(&g_powerThread, nullptr, powerMonitoringThread, &m_powerlistener);
    if (result != 0) {
      logger.error() << "Failed to create power monitoring thread:" << result;
      g_powerListener = nullptr;
    } else {
      logger.debug() << "Power monitoring enabled";
    }
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
  
  logger.debug() << "MacOSNetworkWatcher started successfully";
}

void MacOSNetworkWatcher::checkInterface() {
  logger.debug() << "Checking interface";

  if (!isActive()) {
    logger.debug() << "Feature disabled";
    return;
  }

  // Use wdutil to get reliable WiFi information
  QProcess process;
  process.start("wdutil", QStringList() << "info");
  process.waitForFinished(5000);
  
  QString output = process.readAllStandardOutput();
  QString errorOutput = process.readAllStandardError();
  
  logger.debug() << "wdutil exit code:" << process.exitCode();
  
  if (process.exitCode() != 0) {
    logger.debug() << "wdutil failed with exit code:" << process.exitCode();
    return;
  }
  
  // Parse wdutil output to find WiFi connection info
  QStringList lines = output.split('\n');
  QString ssid, interfaceName, security;
  bool wifiSectionFound = false;
  
  for (int i = 0; i < lines.size(); i++) {
    QString trimmedLine = lines[i].trimmed();
    
    if (trimmedLine == "WIFI") {
      wifiSectionFound = true;
      continue;
    }
    
    if (wifiSectionFound) {
      // Stop parsing when we reach next section header (all caps after separator line)
      if (trimmedLine.startsWith("————————")) {
        if (i + 1 < lines.size()) {
          QString nextLine = lines[i + 1].trimmed();
          if (!nextLine.isEmpty() && nextLine.length() > 2 && nextLine.toUpper() == nextLine && nextLine != "WIFI") {
            break;
          }
        }
        continue; // Skip separator lines
      }
      
      if (trimmedLine.startsWith("Interface Name")) {
        QStringList parts = trimmedLine.split(":");
        if (parts.size() >= 2) {
          interfaceName = parts[1].trimmed();
        }
      } else if (trimmedLine.startsWith("SSID")) {
        QStringList parts = trimmedLine.split(":");
        if (parts.size() >= 2) {
          ssid = parts[1].trimmed();
        }
      } else if (trimmedLine.startsWith("Security")) {
        QStringList parts = trimmedLine.split(":");
        if (parts.size() >= 2) {
          security = parts[1].trimmed();
        }
      }
    }
  }
  
  if (!ssid.isEmpty() && !interfaceName.isEmpty()) {
    logger.debug() << "Found active WiFi connection on" << interfaceName 
                   << "SSID:" << ssid << "Security:" << security;
  } else {
    logger.debug() << "No active WiFi connection found";
  }
}

