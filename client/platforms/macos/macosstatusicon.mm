/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosstatusicon.h"
#include "leakdetector.h"
#include "logger.h"

#import <Cocoa/Cocoa.h>
#import <UserNotifications/UserNotifications.h>
#import <QResource>

#include <QAction>

@interface MacOSStatusIconMenuTarget : NSObject {
 @public
  QAction* action;
}
- (void)triggerAction:(id)sender;
@end

@implementation MacOSStatusIconMenuTarget
- (void)triggerAction:(id)sender {
  Q_UNUSED(sender);
  if (action) {
    action->trigger();
  }
}
@end

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
@property(retain) NSMenu* nativeMenu;
@property(retain) NSMutableArray* menuActionTargets;

- (void)setIcon:(NSData*)imageData;
- (void)setIndicator;
- (void)setIndicatorColor:(NSColor*)color;
- (void)setToolTip:(NSString*)tooltip;
- (void)rebuildMenuFromQMenu:(QMenu*)menu;
@end

@implementation MacOSStatusIconDelegate
- (id)init {
  self = [super init];
  self.menuActionTargets = [[NSMutableArray alloc] init];

  self.statusItem =
      [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
  self.statusItem.visible = true;
  [self setIndicator];

  return self;
}

- (void)dealloc {
  self.nativeMenu = nil;
  self.menuActionTargets = nil;
  [super dealloc];
}

- (void)setIcon:(NSData*)imageData {
  NSImage* image = [[NSImage alloc] initWithData:imageData];
  [image setTemplate:true];

  [self.statusItem.button setImage:image];
  [image release];
}

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

- (void)setIndicatorColor:(NSColor*)color {
  if (self.statusIndicator) {
    self.statusIndicator.layer.backgroundColor = color.CGColor;
  }
}

- (void)setToolTip:(NSString*)tooltip {
  [self.statusItem.button setToolTip:tooltip];
}

- (void)rebuildMenuFromQMenu:(QMenu*)menu {
  [self.menuActionTargets removeAllObjects];

  if (self.nativeMenu) {
    [self.statusItem setMenu:nil];
    self.nativeMenu = nil;
  }

  if (!menu) {
    return;
  }

  NSMenu* nsMenu = [[NSMenu alloc] initWithTitle:@""];
  for (QAction* action : menu->actions()) {
    if (action->isSeparator()) {
      [nsMenu addItem:[NSMenuItem separatorItem]];
      continue;
    }

    MacOSStatusIconMenuTarget* target = [[MacOSStatusIconMenuTarget alloc] init];
    target->action = action;
    [self.menuActionTargets addObject:target];
    [target release];

    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:action->text().toNSString()
                                                  action:@selector(triggerAction:)
                                           keyEquivalent:@""];
    [item setTarget:target];
    [item setEnabled:action->isEnabled()];
    [item setHidden:!action->isVisible()];
    [nsMenu addItem:item];
    [item release];
  }

  self.nativeMenu = nsMenu;
  [self.statusItem setMenu:nsMenu];
}
@end

namespace {
Logger logger("MacOSStatusIcon");

MacOSStatusIconDelegate* m_statusBarIcon = nullptr;
}  // namespace

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

void MacOSStatusIcon::setIconFromData(const QByteArray& imageData) {
  logger.debug() << "Set icon from rendered data";

  if (imageData.isEmpty()) {
    return;
  }

  NSData* data = [NSData dataWithBytes:imageData.constData() length:imageData.size()];
  [m_statusBarIcon setIcon:data];
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

void MacOSStatusIcon::setMenu(QMenu* menu) {
  m_qtMenu = menu;
  rebuildNativeMenu();

  if (menu) {
    connect(menu, &QMenu::aboutToShow, this, [this]() { rebuildNativeMenu(); });
  }
}

void MacOSStatusIcon::rebuildNativeMenu() {
  [m_statusBarIcon rebuildMenuFromQMenu:m_qtMenu.data()];
}

void MacOSStatusIcon::setToolTip(const QString& tooltip) {
  logger.debug() << "Set tooltip";
  [m_statusBarIcon setToolTip:tooltip.toNSString()];
}

void MacOSStatusIcon::showMessage(const QString& title, const QString& message) {
  logger.debug() << "Show message";

  UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];

  [center requestAuthorizationWithOptions:(UNAuthorizationOptionSound | UNAuthorizationOptionAlert |
                                           UNAuthorizationOptionBadge)
                        completionHandler:^(__unused BOOL granted, NSError* _Nullable error) {
                          if (error) {
                            NSLog(@"Error asking for permission to send notifications %@", error);
                          }
                        }];

  UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];

  content.title = [title.toNSString() autorelease];
  content.body = [message.toNSString() autorelease];
  content.sound = [UNNotificationSound defaultSound];

  UNTimeIntervalNotificationTrigger* trigger =
      [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:1 repeats:NO];

  UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:@"amneziavpn"
                                                                        content:content
                                                                        trigger:trigger];

  [center addNotificationRequest:request
           withCompletionHandler:^(NSError* _Nullable error) {
             if (error) {
               logger.error() << "Local Notification failed" << error;
             }
           }];
}
