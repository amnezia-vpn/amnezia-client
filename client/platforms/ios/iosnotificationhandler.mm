/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "platforms/ios/iosnotificationhandler.h"

#import <UserNotifications/UserNotifications.h>
#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

@interface IOSNotificationDelegate
    : UIResponder <UIApplicationDelegate, UNUserNotificationCenterDelegate> {
  IOSNotificationHandler* m_iosNotificationHandler;
}
@end

@implementation IOSNotificationDelegate

- (id)initWithObject:(IOSNotificationHandler*)notification {
  self = [super init];
  if (self) {
    m_iosNotificationHandler = notification;
  }
  return self;
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
       willPresentNotification:(UNNotification*)notification
         withCompletionHandler:
             (void (^)(UNNotificationPresentationOptions options))completionHandler {
  Q_UNUSED(center)
  completionHandler(UNNotificationPresentationOptionAlert);
}

- (void)userNotificationCenter:(UNUserNotificationCenter*)center
    didReceiveNotificationResponse:(UNNotificationResponse*)response
             withCompletionHandler:(void (^)())completionHandler {
  Q_UNUSED(center)
  Q_UNUSED(response)
  completionHandler();
}
@end

IOSNotificationHandler::IOSNotificationHandler(QObject* parent) : NotificationHandler(parent) {

  UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
  [center requestAuthorizationWithOptions:(UNAuthorizationOptionSound | UNAuthorizationOptionAlert |
                                           UNAuthorizationOptionBadge)
                        completionHandler:^(BOOL granted, NSError* _Nullable error) {
                          Q_UNUSED(granted);
                          if (!error) {
                            m_delegate = [[IOSNotificationDelegate alloc] initWithObject:this];
                          }
                        }];
}

IOSNotificationHandler::~IOSNotificationHandler() { }

void IOSNotificationHandler::notify(NotificationHandler::Message type, const QString& title,
                                    const QString& message, int timerMsec) {
  Q_UNUSED(type);

  if (!m_delegate) {
    return;
  }

  UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
  content.title = title.toNSString();
  content.body = message.toNSString();
  content.sound = [UNNotificationSound defaultSound];

  int timerSec = timerMsec / 1000;
  UNTimeIntervalNotificationTrigger* trigger =
      [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:timerSec repeats:NO];

  UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:@"amneziavpn"
                                                                        content:content
                                                                        trigger:trigger];

  UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
  center.delegate = id(m_delegate);

  [center addNotificationRequest:request
           withCompletionHandler:^(NSError* _Nullable error) {
             if (error) {
               NSLog(@"Local Notification failed");
             }
           }];
}
