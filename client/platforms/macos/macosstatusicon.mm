/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosstatusicon.h"
#include "leakdetector.h"

#include <QDebug>
#include <QMenu>

#import <Cocoa/Cocoa.h>
#import <UserNotifications/UserNotifications.h>
#import <QResource>

MacOSStatusIcon::MacOSStatusIcon(QObject* parent) : QObject(parent) {
  MZ_COUNT_CTOR(MacOSStatusIcon);

  NSStatusItem* item = [[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength];
  item.visible = YES;
  m_statusItem = [item retain];
}

MacOSStatusIcon::~MacOSStatusIcon() {
  MZ_COUNT_DTOR(MacOSStatusIcon);

  NSStatusItem* item = static_cast<NSStatusItem*>(m_statusItem);
  item.menu = nil;
  [[NSStatusBar systemStatusBar] removeStatusItem:item];
  [item release];
  m_statusItem = nullptr;
}

void MacOSStatusIcon::setIcon(const QString& iconPath) {
  QResource resource(iconPath);
  if (!resource.isValid()) {
    qWarning() << "MacOSStatusIcon: invalid icon resource" << iconPath;
    return;
  }

  NSImage* image = [[NSImage alloc] initWithData:resource.uncompressedData().toNSData()];
  CGFloat side = [[NSStatusBar systemStatusBar] thickness] - 4.0;
  image.size = NSMakeSize(side, side);
  static_cast<NSStatusItem*>(m_statusItem).button.image = image;
  [image release];
}

void MacOSStatusIcon::setMenu(QMenu* menu) {
  static_cast<NSStatusItem*>(m_statusItem).menu = menu ? menu->toNSMenu() : nil;
}

void MacOSStatusIcon::showMessage(const QString& title, const QString& message) {
  UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];

  // This is a no-op if authorization has already been granted.
  [center requestAuthorizationWithOptions:(UNAuthorizationOptionSound | UNAuthorizationOptionAlert |
                                           UNAuthorizationOptionBadge)
                        completionHandler:^(BOOL, NSError* _Nullable error) {
                          if (error) {
                            // Note: this error may happen if the application is not signed.
                            qWarning() << "MacOSStatusIcon: notification authorization error:"
                                       << QString::fromNSString(error.localizedDescription);
                          }
                        }];

  UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
  content.title = title.toNSString();
  content.body = message.toNSString();
  content.sound = [UNNotificationSound defaultSound];

  UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:@"amneziavpn"
                                                                        content:content
                                                                        trigger:nil];
  [content release];

  [center addNotificationRequest:request
           withCompletionHandler:^(NSError* _Nullable error) {
             if (error) {
               qWarning() << "MacOSStatusIcon: local notification failed:"
                          << QString::fromNSString(error.localizedDescription);
             }
           }];
}
