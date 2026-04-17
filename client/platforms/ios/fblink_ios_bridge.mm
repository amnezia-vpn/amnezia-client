#import <Foundation/Foundation.h>
#import <FBLink-Swift.h>

#include "fblink_ios_bridge.h"

namespace {
NSString *toNSString(const std::string &value)
{
    if (value.empty()) {
        return @"";
    }

    NSString *result = [[NSString alloc] initWithBytes:value.data() length:value.size() encoding:NSUTF8StringEncoding];
    return result ?: @"";
}
}

namespace FBLink {
std::string swiftUpdateLogData(const std::string &qtString)
{
    @autoreleasepool {
        NSString *swiftResult = [FBLinkIOSBridge swiftUpdateLogData:toNSString(qtString)];
        if (!swiftResult) {
            return {};
        }
        const char *utf8 = [swiftResult UTF8String];
        return utf8 ? std::string(utf8) : std::string();
    }
}

void swiftDeleteLog()
{
    @autoreleasepool {
        [FBLinkIOSBridge swiftDeleteLog];
    }
}

void toggleLogging(bool isEnabled)
{
    @autoreleasepool {
        [FBLinkIOSBridge toggleLogging:isEnabled];
    }
}

void clearSettings()
{
    @autoreleasepool {
        [FBLinkIOSBridge clearSettings];
    }
}

void toggleScreenshots(bool isEnabled)
{
    @autoreleasepool {
        [FBLinkIOSBridge toggleScreenshots:isEnabled];
    }
}

void removeVPNC(const std::string &vpncName)
{
    @autoreleasepool {
        [FBLinkIOSBridge removeVPNC:toNSString(vpncName)];
    }
}
}
