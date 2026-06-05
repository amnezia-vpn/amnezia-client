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
 * Creates a NSStatusItem that holds the tray icon. The icon is set as a
 * template image, so the system controls its effective appearance for the
 * current menu bar theme. The connection status is baked into the artwork, so
 * no separate colored indicator is drawn.
 */
@interface MacOSStatusIconDelegate : NSObject
@property(assign) NSStatusItem* statusItem;
@property(retain) NSMenu* nativeMenu;
@property(retain) NSMutableArray* menuActionTargets;

- (void)setIcon:(NSData*)imageData asTemplate:(BOOL)asTemplate;
- (void)setToolTip:(NSString*)tooltip;
- (void)rebuildMenuFromQMenu:(QMenu*)menu;
@end

@implementation MacOSStatusIconDelegate
/**
 * Initializes and sets the status item and indicator objects.
 *
 * @return An instance of MacOSStatusIconDelegate.
 */
- (id)init {
  self = [super init];
  self.menuActionTargets = [[NSMutableArray alloc] init];

  // Create status item
  self.statusItem =
      [[[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength] retain];
  self.statusItem.visible = true;

  return self;
}

- (void)dealloc {
  self.nativeMenu = nil;
  self.menuActionTargets = nil;
  [super dealloc];
}

/**
 * Sets the image for the status icon.
 *
 * @param imageData The data for the icon image.
 * @param asTemplate When true the icon is a template image recolored by the
 *        system for the current menu bar appearance. When false the icon is
 *        rendered in its original colors (used for the colored error icon).
 */
- (void)setIcon:(NSData*)imageData asTemplate:(BOOL)asTemplate {
  NSImage* image = [[NSImage alloc] initWithData:imageData];
  [image setTemplate:asTemplate];

  [self.statusItem.button setImage:image];
  [image release];
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

  [m_statusBarIcon setIcon:imageResource.uncompressedData().toNSData() asTemplate:true];
}

void MacOSStatusIcon::setIconFromData(const QByteArray& imageData, bool asTemplate) {
  logger.debug() << "Set icon from rendered data";

  if (imageData.isEmpty()) {
    return;
  }

  NSData* data = [NSData dataWithBytes:imageData.constData() length:imageData.size()];
  [m_statusBarIcon setIcon:data asTemplate:asTemplate];
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

  // This is a no-op is authorization has been granted.
  [center requestAuthorizationWithOptions:(UNAuthorizationOptionSound | UNAuthorizationOptionAlert |
                                           UNAuthorizationOptionBadge)
                        completionHandler:^(__unused BOOL granted, NSError* _Nullable error) {
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
