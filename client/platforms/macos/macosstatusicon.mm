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
@property(assign) NSSize preferredImageSize;
@property(assign) BOOL hasPreferredImageSize;

- (void)setIcon:(NSData*)imageData;
- (void)setIcon:(NSData*)imageData isTemplate:(BOOL)isTemplate;
- (void)setLength:(CGFloat)length;
- (void)setImageSize:(NSSize)size;
- (void)configureButton;
- (void)logButtonMetrics:(NSString*)reason;
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
      [[[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength] retain];
  self.statusItem.visible = true;
  [self configureButton];

  return self;
}

/**
 * Sets the image for the status icon.
 *
 * @param iconPath The data for the icon image.
 */
- (void)setIcon:(NSData*)imageData {
  [self setIcon:imageData isTemplate:true];
}

- (void)setIcon:(NSData*)imageData isTemplate:(BOOL)isTemplate {
  NSImage* image = [[NSImage alloc] initWithData:imageData];
  [image setTemplate:isTemplate];
  if (self.hasPreferredImageSize) {
    [image setSize:self.preferredImageSize];
  }

  [self.statusItem.button setImage:image];
  [self.statusItem.button setImagePosition:NSImageOnly];
  [self.statusItem.button setImageScaling:NSImageScaleProportionallyUpOrDown];
  [self logButtonMetrics:@"setIcon"];
  [image release];
}

- (void)setLength:(CGFloat)length {
  self.statusItem.length = length;
  [self logButtonMetrics:@"setLength"];
}

- (void)setImageSize:(NSSize)size {
  self.preferredImageSize = size;
  self.hasPreferredImageSize = YES;

  NSImage* image = self.statusItem.button.image;
  if (!image) {
    [self logButtonMetrics:@"setImageSize(pending)"];
    return;
  }

  [image setSize:size];
  [self.statusItem.button setImage:image];
  [self logButtonMetrics:@"setImageSize"];
}

/**
 * Configures the status item button to behave like a compact image-only menu
 * bar item without AppKit's wide pressed highlight.
 */
- (void)configureButton {
  NSStatusBarButton* button = self.statusItem.button;
  [button setImagePosition:NSImageOnly];
  [button setImageScaling:NSImageScaleProportionallyUpOrDown];
  [button setBordered:NO];

  NSButtonCell* cell = button.cell;
  if ([cell respondsToSelector:@selector(setHighlightsBy:)]) {
    [cell setHighlightsBy:NSNoCellMask];
  }

  [self logButtonMetrics:@"configureButton"];
}

- (void)logButtonMetrics:(NSString*)reason {
  NSStatusBarButton* button = self.statusItem.button;
  NSRect frame = button.frame;
  NSRect bounds = button.bounds;
  NSSize imageSize = button.image ? button.image.size : NSZeroSize;
  NSLog(@"[DEBUG] Amnezia MacOSStatusIconDelegate : %@ length=%.2f frame=(%.2f %.2f %.2f %.2f) bounds=(%.2f %.2f %.2f %.2f) image=(%.2f %.2f)",
        reason,
        self.statusItem.length,
        frame.origin.x,
        frame.origin.y,
        frame.size.width,
        frame.size.height,
        bounds.origin.x,
        bounds.origin.y,
        bounds.size.width,
        bounds.size.height,
        imageSize.width,
        imageSize.height);
}

/**
 * Sets the color if the indicator.
 *
 * @param color The indicator background color.
 */
- (void)setIndicatorColor:(NSColor*)color {
  Q_UNUSED(color);
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

void MacOSStatusIcon::setIconData(const QByteArray& imageData, bool isTemplate) {
  logger.debug() << "Set icon data. Bytes:" << imageData.size() << "template:" << isTemplate;
  [m_statusBarIcon setIcon:imageData.toNSData() isTemplate:isTemplate];
}

void MacOSStatusIcon::setLength(qreal length) {
  logger.debug() << "Set status item length:" << length;
  [m_statusBarIcon setLength:length];
}

void MacOSStatusIcon::setImageSize(qreal width, qreal height) {
  logger.debug() << "Set status image size:" << width << "x" << height;
  [m_statusBarIcon setImageSize:NSMakeSize(width, height)];
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
