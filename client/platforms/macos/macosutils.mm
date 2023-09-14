/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosutils.h"
#include "logger.h"

#include <objc/message.h>
#include <objc/objc.h>

#import <Cocoa/Cocoa.h>
#import <ServiceManagement/ServiceManagement.h>

namespace {
Logger logger("MacOSUtils");
}

// static
NSString* MacOSUtils::appId() {
  NSString* appId = [[NSBundle mainBundle] bundleIdentifier];
  if (!appId) {
    // Fallback. When an unsigned/un-notarized app is executed in
    // command-line mode, it could fail the fetching of its own bundle id.
    appId = @"org.amnezia.AmneziaVPN";
  }

  return appId;
}

// static
QString MacOSUtils::computerName() {
  NSString* name = [[NSHost currentHost] localizedName];
  return QString::fromNSString(name);
}

// static
void MacOSUtils::enableLoginItem(bool startAtBoot) {
  logger.debug() << "Enabling login-item";

  NSString* appId = MacOSUtils::appId();
  Q_ASSERT(appId);

  NSString* loginItemAppId =
      QString("%1.login-item").arg(QString::fromNSString(appId)).toNSString();
  CFStringRef cfs = (__bridge CFStringRef)loginItemAppId;

  Boolean ok = SMLoginItemSetEnabled(cfs, startAtBoot ? YES : NO);
  logger.debug() << "Result: " << ok;
}

namespace {

bool dockClickHandler(id self, SEL cmd, ...) {
  Q_UNUSED(self);
  Q_UNUSED(cmd);

  logger.debug() << "Dock icon clicked.";
  
  //TODO IMPL FOR AMNEZIA
  //QmlEngineHolder::instance()->showWindow();
  return FALSE;
}

}  // namespace

// static
void MacOSUtils::setDockClickHandler() {
  NSApplication* app = [NSApplication sharedApplication];
  if (!app) {
    logger.debug() << "No sharedApplication";
    return;
  }

  id delegate = [app delegate];
  if (!delegate) {
    logger.debug() << "No delegate";
    return;
  }

  Class delegateClass = [delegate class];
  if (!delegateClass) {
    logger.debug() << "No delegate class";
    return;
  }

  SEL shouldHandle = sel_registerName("applicationShouldHandleReopen:hasVisibleWindows:");
  if (class_getInstanceMethod(delegateClass, shouldHandle)) {
    if (!class_replaceMethod(delegateClass, shouldHandle, (IMP)dockClickHandler, "B@:")) {
      logger.error() << "Failed to replace the dock click handler";
    }
  } else if (!class_addMethod(delegateClass, shouldHandle, (IMP)dockClickHandler, "B@:")) {
    logger.error() << "Failed to register the dock click handler";
  }
}

void MacOSUtils::hideDockIcon() {
  [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
}

void MacOSUtils::showDockIcon() {
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
}

/**
 * Replace the setImage method on NSStatusBarButton with a method that scales
 * images proportionally before setting.
 *
 * The reason for this is that there is a bug in Qt 5.15 that causes status bar
 * icons to be displayed larger than UI recommendations, and out of proportion
 * on displays with a device pixel ratio greater than 1 (MacOS Big Sur only).
 * This bug will not be fixed in Qt open source versions, so we have to resort
 * to a hack that exchanges the implementation of a method on NSStatusBarButton
 * with one that correctly scales the icon.
 *
 * Original bug (and sample implementation):
 * https://bugreports.qt.io/browse/QTBUG-88600
 */
void MacOSUtils::patchNSStatusBarSetImageForBigSur() {
  Method original = class_getInstanceMethod([NSStatusBarButton class], @selector(setImage:));
  Method patched = class_getInstanceMethod([NSStatusBarButton class], @selector(setImagePatched:));
  method_exchangeImplementations(original, patched);
}

@interface NSImageScalingHelper : NSObject
/**
 * Create a proportionally scaled image according to the given target size.
 *
 * @param sourceImage The original image to be scaled.
 * @param targetSize The required size of the image.
 * @return A scaled image.
 */
+ (NSImage*)imageByScaling:(NSImage*)sourceImage size:(NSSize)targetSize;
@end

@implementation NSImageScalingHelper
+ (NSImage*)imageByScaling:(NSImage*)sourceImage size:(NSSize)targetSize {
  NSImage* newImage = nil;

  if ([sourceImage isValid]) {
    NSSize sourceSize = [sourceImage size];

    if (sourceSize.width != 0.0 && sourceSize.height != 0.0) {
      float scaleFactor = 0.0;
      float scaledWidth = targetSize.width;
      float scaledHeight = targetSize.height;

      NSPoint thumbnailPoint = NSZeroPoint;

      if (NSEqualSizes(sourceSize, targetSize) == NO) {
        float widthFactor = targetSize.width / sourceSize.width;
        float heightFactor = targetSize.height / sourceSize.height;

        if (widthFactor < heightFactor) {
          scaleFactor = widthFactor;
        } else {
          scaleFactor = heightFactor;
        }
        scaledWidth = sourceSize.width * scaleFactor;
        scaledHeight = sourceSize.height * scaleFactor;

        if (widthFactor < heightFactor) {
          thumbnailPoint.y = (targetSize.height - scaledHeight) * 0.5;
        } else {
          thumbnailPoint.x = (targetSize.width - scaledWidth) * 0.5;
        }
      }

      newImage = [[NSImage alloc] initWithSize:targetSize];

      [newImage lockFocus];

      NSRect thumbnailRect;
      thumbnailRect.origin = thumbnailPoint;
      thumbnailRect.size.width = scaledWidth;
      thumbnailRect.size.height = scaledHeight;
      [sourceImage drawInRect:thumbnailRect
                     fromRect:NSZeroRect
                    operation:NSCompositingOperationSourceOver
                     fraction:1.0];

      [newImage unlockFocus];

      [newImage setTemplate:[sourceImage isTemplate]];
    }
  }
  return [newImage autorelease];
}
@end

@implementation NSStatusBarButton (Swizzle)
- (void)setImagePatched:(NSImage*)image {
  NSImage* img = image;

  if (image != nil) {
    int thickness = [[NSStatusBar systemStatusBar] thickness];
    img = [NSImageScalingHelper imageByScaling:image size:NSMakeSize(thickness, thickness)];
  }

  [self setImagePatched:img];
}
@end
