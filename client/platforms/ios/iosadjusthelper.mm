/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "iosadjusthelper.h"
#include "logger.h"
#include "constants.h"

#import <AdjustSdk/Adjust.h>

namespace {

Logger logger(LOG_IOS, "IOSAdjustHelper");

}  // namespace

void IOSAdjustHelper::initialize() {

  NSString *adjustToken = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"ADJUST_SDK_TOKEN"];

  if(adjustToken.length) {
    NSString *environment = Constants::inProduction() ? ADJEnvironmentProduction : ADJEnvironmentSandbox;
    ADJConfig *adjustConfig = [ADJConfig configWithAppToken:adjustToken
                                                environment:environment];
    [adjustConfig setLogLevel:ADJLogLevelDebug];
    [Adjust appDidLaunch:adjustConfig];
  }
}

void IOSAdjustHelper::trackEvent(const QString& eventToken) {
  NSString *adjustToken = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"ADJUST_SDK_TOKEN"];

  if(adjustToken.length) {
    ADJEvent *event = [ADJEvent eventWithEventToken:eventToken.toNSString()];
    [Adjust trackEvent:event];
  }
}
