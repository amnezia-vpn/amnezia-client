/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "macosutils.h"
#include "logger.h"
#include "models/helpmodel.h"
#include "qmlengineholder.h"

#include <objc/message.h>
#include <objc/objc.h>

#include <QFile>
#include <QMenuBar>

#import <Cocoa/Cocoa.h>
#import <ServiceManagement/ServiceManagement.h>

namespace {
Logger logger(LOG_MACOS, "MacOSUtils");
}

// static
QString MacOSUtils::computerName() {
  NSString* name = [[NSHost currentHost] localizedName];
  return QString::fromNSString(name);
}

// static
void MacOSUtils::enableLoginItem(bool startAtBoot) {
  logger.debug() << "Enabling login-item";

  NSString* appId = [[NSBundle mainBundle] bundleIdentifier];
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
  QmlEngineHolder::instance()->showWindow();
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
