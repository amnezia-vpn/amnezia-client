#include "macosAppInfo.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

#include <QFileInfo>
#include <QString>

#include "logger.h"

namespace {
Logger logger("MacOSAppInfo");
}

bool MacOSAppInfo::fillFromPath(const QString &appPath, amnezia::InstalledAppInfo &info)
{
    if (appPath.isEmpty()) {
        logger.debug() << "fillFromPath empty path";
        return false;
    }

    NSString *path = appPath.toNSString();
    NSBundle *bundle = [NSBundle bundleWithPath:path];
    if (bundle == nil) {
        logger.debug() << "fillFromPath no NSBundle at" << appPath;
        return false;
    }

    info.appPath = appPath;
    info.packageName = QString::fromNSString(bundle.bundleIdentifier);

    NSString *displayName = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
    if (displayName.length == 0) {
        displayName = [bundle objectForInfoDictionaryKey:@"CFBundleName"];
    }
    if (displayName.length > 0) {
        info.appName = QString::fromNSString(displayName);
    } else {
        info.appName = QFileInfo(appPath).completeBaseName();
    }

    logger.debug() << "fillFromPath identifier=" << QString::fromNSString(bundle.bundleIdentifier)
                   << "name=" << info.appName
                   << "bundleId=" << info.packageName
                   << "path=" << info.appPath;
    return !info.packageName.isEmpty() || !info.appPath.isEmpty();
}

QString MacOSAppInfo::pickApplication()
{
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    panel.canChooseFiles = YES;
    panel.canChooseDirectories = NO;
    panel.allowsMultipleSelection = NO;
    panel.treatsFilePackagesAsDirectories = NO;
    panel.directoryURL = [NSURL fileURLWithPath:@"/Applications"];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    panel.allowedFileTypes = @[ @"app" ];
#pragma clang diagnostic pop
    panel.title = @"Select Application";

    if ([panel runModal] != NSModalResponseOK) {
        return {};
    }

    NSURL *url = panel.URL;
    if (url == nil) {
        logger.debug() << "pickApplication cancelled or no URL";
        return {};
    }
    const QString path = QString::fromNSString(url.path);
    logger.debug() << "pickApplication path=" << path;
    return path;
}
