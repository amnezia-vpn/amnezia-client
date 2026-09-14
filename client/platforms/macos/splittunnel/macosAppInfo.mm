#include "macosAppInfo.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <Security/Security.h>

#include <QFileInfo>
#include <QString>

#include "logger.h"

namespace {
Logger logger("MacOSAppInfo");

QString QStr(NSString *value)
{
    return value != nil ? QString::fromNSString(value) : QStringLiteral("(nil)");
}

/*! Designated requirement of the bundle, logged so a mismatch between the
 *  picked app and the flows we later see can be diagnosed. */
QString DesignatedRequirement(NSString *path)
{
    NSURL *url = [NSURL fileURLWithPath:path];
    SecStaticCodeRef code = NULL;
    OSStatus status = SecStaticCodeCreateWithPath((__bridge CFURLRef)url, kSecCSDefaultFlags, &code);
    if (status != errSecSuccess || code == NULL) {
        return QStringLiteral("<SecStaticCodeCreateWithPath failed: %1>").arg(status);
    }

    SecRequirementRef requirement = NULL;
    status = SecCodeCopyDesignatedRequirement(code, kSecCSDefaultFlags, &requirement);
    CFRelease(code);
    if (status != errSecSuccess || requirement == NULL) {
        return QStringLiteral("<SecCodeCopyDesignatedRequirement failed: %1>").arg(status);
    }

    CFStringRef text = NULL;
    status = SecRequirementCopyString(requirement, kSecCSDefaultFlags, &text);
    CFRelease(requirement);
    if (status != errSecSuccess || text == NULL) {
        return QStringLiteral("<SecRequirementCopyString failed: %1>").arg(status);
    }

    const QString result = QString::fromCFString(text);
    CFRelease(text);
    return result;
}
} // namespace

bool MacOSAppInfo::fillFromPath(const QString &appPath, amnezia::InstalledAppInfo &info)
{
    logger.debug() << "fillFromPath:" << appPath;

    if (appPath.isEmpty()) {
        logger.error() << "fillFromPath: empty path";
        return false;
    }

    NSString *path = appPath.toNSString();
    NSBundle *bundle = [NSBundle bundleWithPath:path];
    if (bundle == nil) {
        logger.error() << "fillFromPath: no NSBundle at" << appPath
                       << "- only .app bundles can be excluded";
        return false;
    }

    info.appPath = appPath;
    info.packageName = QStr(bundle.bundleIdentifier);
    if (bundle.bundleIdentifier == nil) {
        info.packageName.clear();
        logger.error() << "fillFromPath:" << appPath
                       << "has no CFBundleIdentifier - matching will fall back to the executable path";
    }

    NSString *displayName = [bundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
    if (displayName.length == 0) {
        displayName = [bundle objectForInfoDictionaryKey:@"CFBundleName"];
    }
    if (displayName.length > 0) {
        info.appName = QString::fromNSString(displayName);
    } else {
        info.appName = QFileInfo(appPath).completeBaseName();
        logger.debug() << "fillFromPath: no CFBundleDisplayName/CFBundleName, using file name"
                       << info.appName;
    }

    logger.info() << "picked app name=" << info.appName
                  << "bundleId=" << info.packageName
                  << "path=" << info.appPath
                  << "executable=" << QStr(bundle.executablePath)
                  << "version=" << QStr([bundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"]);
    logger.debug() << "designated requirement:" << DesignatedRequirement(path);

    const bool ok = !info.packageName.isEmpty() || !info.appPath.isEmpty();
    if (!ok) {
        logger.error() << "fillFromPath: neither bundle id nor path could be determined for" << appPath;
    }
    return ok;
}

QString MacOSAppInfo::pickApplication()
{
    logger.debug() << "pickApplication: opening panel";

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

    const NSModalResponse response = [panel runModal];
    if (response != NSModalResponseOK) {
        logger.debug() << "pickApplication: cancelled (response=" << (long)response << ")";
        return {};
    }

    NSURL *url = panel.URL;
    if (url == nil) {
        logger.error() << "pickApplication: panel returned OK but no URL";
        return {};
    }
    const QString path = QString::fromNSString(url.path);
    logger.info() << "pickApplication: selected" << path;
    return path;
}
