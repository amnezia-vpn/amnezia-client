/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MACOSNETWORKWATCHER_H
#define MACOSNETWORKWATCHER_H

#import <Network/Network.h>

#include "../ios/iosnetworkwatcher.h"
#include "networkwatcherimpl.h"

#include <IOKit/pwr_mgt/IOPMLib.h>
#include <IOKit/IOMessage.h>


class QString;

// Inspired by https://ladydebug.com/blog/2020/05/21/programmatically-capture-energy-saver-event-on-mac/
class PowerNotificationsListener
{
public:
    PowerNotificationsListener(class MacOSNetworkWatcher* watcher) : m_watcher(watcher) {}
    void registerForNotifications();
    void cleanup();

private:
    static void sleepWakeupCallBack(void *refParam, io_service_t service, natural_t messageType, void *messageArgument);

private:
    class MacOSNetworkWatcher* m_watcher = nullptr;
    IONotificationPortRef notifyPortRef = nullptr; // notification port allocated by IORegisterForSystemPower
    io_object_t notifierObj = IO_OBJECT_NULL; // notifier object, used to deregister later
    io_connect_t rootPowerDomain = IO_OBJECT_NULL; // a reference to the Root Power Domain IOService
};


class MacOSNetworkWatcher final : public IOSNetworkWatcher {
 public:
  MacOSNetworkWatcher(QObject* parent);
  ~MacOSNetworkWatcher();

  void start() override;

  void checkInterface();

  void controllerStateChanged();

 private:
  void* m_delegate = nullptr;
  PowerNotificationsListener m_powerlistener;
};

#endif  // MACOSNETWORKWATCHER_H
