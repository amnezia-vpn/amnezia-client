/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosstatusicon.h"
#include "leakdetector.h"
#include "logger.h"

#import <Cocoa/Cocoa.h>
#import <UserNotifications/UserNotifications.h>
#import <QResource>

/**
 * Creates a NSStatusItem with that can hold an icon. Additionally a NSView is
 * set as a subview to the button item of the status item. The view serves as
 * an indicator that can be displayed in color eventhough the icon is set as a
 * template. In that way we give the system control over it’s effective
 * appearance.
 */
@interface MacOSStatusIconDelegate : NSObject
@property(assign) NSStatusItem* statusItem;
@property(assign) NSView* statusIndicator;

- (void)setIcon:(NSData*)imageData;
- (void)setIndicator;
- (void)setIndicatorColor:(NSColor*)color;
- (void)setMenu:(NSMenu*)statusBarMenu;
- (void)setToolTip:(NSString*)tooltip;
@end

@implementation MacOSStatusIconDelegate
/**
 * Initializes and sets the status item and indicator objects.
 *
 * @return An instance of MacOSStatusIconDelegate.
 */
- (id)init {
  self = [super init];

  // Create status item
  self.statusItem =
      [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
  self.statusItem.visible = true;
  // Add the indicator as a subview
  [self setIndicator];

  return self;
}

/**
 * Sets the image for the status icon.
 *
 * @param iconPath The data for the icon image.
 */
- (void)setIcon:(NSData*)imageData {
  NSImage* image = [[NSImage alloc] initWithData:imageData];
  [image setTemplate:true];

  [self.statusItem.button setImage:image];
  [image release];
}

/**
 * Adds status indicator as a subview to the status item button.
 */
- (void)setIndicator {
  float viewHeight = NSHeight([self.statusItem.button bounds]);
  float dotSize = viewHeight * 0.35;
  float dotOrigin = (viewHeight - dotSize) * 0.8;

  NSView* dot = [[NSView alloc] initWithFrame:NSMakeRect(dotOrigin, dotOrigin, dotSize, dotSize)];
  self.statusIndicator = dot;
  self.statusIndicator.wantsLayer = true;
  self.statusIndicator.layer.cornerRadius = dotSize * 0.5;

  [self.statusItem.button addSubview:self.statusIndicator];
  [dot release];
}

/**
 * Sets the color if the indicator.
 *
 * @param color The indicator background color.
 */
- (void)setIndicatorColor:(NSColor*)color {
  if (self.statusIndicator) {
    self.statusIndicator.layer.backgroundColor = color.CGColor;
  }
}

/**
 * Sets the status bar menu to the status item.
 *
 * @param statusBarMenu The menu object that is passed from QT.
 */
- (void)setMenu:(NSMenu*)statusBarMenu {
  [self.statusItem setMenu:statusBarMenu];
}

/**
 * Sets the tooltip string for the status item.
 *
 * @param tooltip The tooltip string.
 */
- (void)setToolTip:(NSString*)tooltip {
  [self.statusItem.button setToolTip:tooltip];
}
@end

namespace {
Logger logger("MacOSStatusIcon");

MacOSStatusIconDelegate* m_statusBarIcon = nullptr;
}

MacOSStatusIcon::MacOSStatusIcon(QObject* parent) : QObject(parent) {
  MZ_COUNT_CTOR(MacOSStatusIcon);

  logger.debug() << "Register delegate";
  Q_ASSERT(!m_statusBarIcon);

  m_statusBarIcon = [[MacOSStatusIconDelegate alloc] init];
}

MacOSStatusIcon::~MacOSStatusIcon() {
  MZ_COUNT_DTOR(MacOSStatusIcon);

  logger.debug() << "Remove delegate";
  Q_ASSERT(m_statusBarIcon);

  [static_cast<MacOSStatusIconDelegate*>(m_statusBarIcon) dealloc];
  m_statusBarIcon = nullptr;
}

void MacOSStatusIcon::setIcon(const QString& iconPath) {
  logger.debug() << "Set icon" << iconPath;

  QResource imageResource = QResource(iconPath);
  Q_ASSERT(imageResource.isValid());

  [m_statusBarIcon setIcon:imageResource.uncompressedData().toNSData()];
}

void MacOSStatusIcon::setIndicatorColor(const QColor& indicatorColor) {
  logger.debug() << "Set indicator color";

  if (!indicatorColor.isValid()) {
    [m_statusBarIcon setIndicatorColor:[NSColor clearColor]];
    return;
  }

  NSColor* color = [NSColor colorWithCalibratedRed:indicatorColor.red() / 255.0f
                                             green:indicatorColor.green() / 255.0f
                                              blue:indicatorColor.blue() / 255.0f
                                             alpha:indicatorColor.alpha() / 255.0f];
  [m_statusBarIcon setIndicatorColor:color];
}

void MacOSStatusIcon::setMenu(NSMenu* statusBarMenu) {
  logger.debug() << "Set menu";
  [m_statusBarIcon setMenu:statusBarMenu];
}

void MacOSStatusIcon::setToolTip(const QString& tooltip) {
  logger.debug() << "Set tooltip";
  [m_statusBarIcon setToolTip:tooltip.toNSString()];
}

void MacOSStatusIcon::showMessage(const QString& title, const QString& message) {
  logger.debug() << "Show message";

  UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];

  // This is a no-op is authorization has been granted.
  [center requestAuthorizationWithOptions:(UNAuthorizationOptionSound | UNAuthorizationOptionAlert |
                                           UNAuthorizationOptionBadge)
                        completionHandler:^(BOOL granted, NSError* _Nullable error) {
                          if (error) {
                            // Note: This error may happen if the application is not signed.
                            NSLog(@"Error asking for permission to send notifications %@", error);
                            return;
                          }
                        }];

  UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];

  content.title = [title.toNSString() autorelease];
  content.body = [message.toNSString() autorelease];
  content.sound = [UNNotificationSound defaultSound];

  UNTimeIntervalNotificationTrigger* trigger =
      [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:1 repeats:NO];

  UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:@"mozillavpn"
                                                                        content:content
                                                                        trigger:trigger];

  [center addNotificationRequest:request
           withCompletionHandler:^(NSError* _Nullable error) {
             if (error) {
               logger.error() << "Local Notification failed" << error;
             }
           }];
}
