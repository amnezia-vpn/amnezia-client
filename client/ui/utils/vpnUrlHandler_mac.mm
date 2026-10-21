#include "vpnUrlHandler.h"

#import <AppKit/AppKit.h>
#import <CoreServices/CoreServices.h>

namespace {
NSURL *probeUrl()
{
    return [NSURL URLWithString:@"vpn://probe"];
}
} // namespace

#ifdef MACOS_NE

VpnUrlHandler::Status VpnUrlHandler::status()
{
    @autoreleasepool {
        NSURL *handler = [[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:probeUrl()];
        if (!handler) {
            return Status::NotRegistered;
        }
        NSURL *ours = [[NSBundle mainBundle] bundleURL];
        return [handler.URLByStandardizingPath isEqual:ours.URLByStandardizingPath] ? Status::IsDefault
                                                                                    : Status::OtherHandler;
    }
}

bool VpnUrlHandler::setAsDefault()
{
    @autoreleasepool {
        [[NSWorkspace sharedWorkspace] setDefaultApplicationAtURL:[[NSBundle mainBundle] bundleURL]
                                             toOpenURLsWithScheme:@"vpn"
                                                completionHandler:^(NSError *error) {
                                                }];
        return true;
    }
}

void VpnUrlHandler::unregister()
{
}

#else

namespace {
NSString *const kLsRegister =
        @"/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/Support/lsregister";

NSURL *templateUrl()
{
    return [[[NSBundle mainBundle] resourceURL] URLByAppendingPathComponent:@"LinkHandler.template" isDirectory:YES];
}

NSString *helperBundleId()
{
    return [NSBundle bundleWithURL:templateUrl()].bundleIdentifier;
}

NSURL *dataFolderUrl()
{
    NSURL *appSupport = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory
                                                               inDomains:NSUserDomainMask].firstObject;
    return [appSupport URLByAppendingPathComponent:@"AmneziaVPN" isDirectory:YES];
}

NSURL *copyUrl()
{
    return [[dataFolderUrl() URLByAppendingPathComponent:@"LinkHandler" isDirectory:YES]
            URLByAppendingPathComponent:@"AmneziaVPN.app"
                            isDirectory:YES];
}

bool sameFile(NSURL *a, NSURL *b)
{
    return [a.URLByResolvingSymlinksInPath.path isEqualToString:b.URLByResolvingSymlinksInPath.path];
}

bool isOurHelper(NSURL *app)
{
    NSString *helperId = helperBundleId();
    return sameFile(app, copyUrl())
            || (helperId.length && [[NSBundle bundleWithURL:app].bundleIdentifier isEqualToString:helperId]);
}

bool runTool(NSString *path, NSArray<NSString *> *args)
{
    NSTask *task = [NSTask launchedTaskWithExecutableURL:[NSURL fileURLWithPath:path]
                                               arguments:args
                                                   error:nil
                                      terminationHandler:nil];
    if (!task) {
        return false;
    }
    [task waitUntilExit];
    return task.terminationStatus == 0;
}

bool sameContents(NSURL *a, NSURL *b, NSString *relativePath)
{
    NSData *left = [NSData dataWithContentsOfURL:[a URLByAppendingPathComponent:relativePath]];
    NSData *right = [NSData dataWithContentsOfURL:[b URLByAppendingPathComponent:relativePath]];
    if (!left || !right) {
        return !left && !right;
    }
    return [left isEqualToData:right];
}

bool isCopyUpToDate()
{
    NSURL *tmpl = templateUrl();
    NSURL *copy = copyUrl();
    NSString *executable = [[NSBundle bundleWithURL:tmpl] objectForInfoDictionaryKey:@"CFBundleExecutable"];
    if (!executable.length) {
        return false;
    }
    NSString *executablePath = [@"Contents/MacOS" stringByAppendingPathComponent:executable];
    return [[NSFileManager defaultManager] fileExistsAtPath:[copy URLByAppendingPathComponent:executablePath].path]
            && sameContents(tmpl, copy, @"Contents/Info.plist")
            && sameContents(tmpl, copy, executablePath)
            && sameContents(tmpl, copy, @"Contents/_CodeSignature/CodeResources");
}

void removeHelper(NSURL *app)
{
    runTool(kLsRegister, @[ @"-u", app.path ]);

    NSString *folder = [dataFolderUrl().URLByResolvingSymlinksInPath.path stringByAppendingString:@"/"];
    if (![app.URLByResolvingSymlinksInPath.path hasPrefix:folder]
        || ![[NSFileManager defaultManager] fileExistsAtPath:app.path]) {
        return;
    }
    [[NSFileManager defaultManager] removeItemAtURL:app error:nil];
}

void removeHelpers(NSURL *keep)
{
    NSMutableArray<NSURL *> *apps =
            [[[NSWorkspace sharedWorkspace] URLsForApplicationsToOpenURL:probeUrl()] mutableCopy] ?: [NSMutableArray array];
    [apps addObject:copyUrl()];
    for (NSURL *app in apps) {
        if (keep && sameFile(app, keep)) {
            continue;
        }
        if (isOurHelper(app) && [[NSFileManager defaultManager] fileExistsAtPath:app.path]) {
            removeHelper(app);
        }
    }
}

bool ensureCopy()
{
    NSURL *tmpl = templateUrl();
    if (![[NSFileManager defaultManager] fileExistsAtPath:tmpl.path]) {
        return false;
    }
    if (isCopyUpToDate()) {
        return true;
    }

    removeHelpers(nil);
    NSURL *copy = copyUrl();
    if (![[NSFileManager defaultManager] createDirectoryAtURL:copy.URLByDeletingLastPathComponent
                                  withIntermediateDirectories:YES
                                                   attributes:nil
                                                        error:nil]) {
        return false;
    }
    return runTool(@"/usr/bin/ditto", @[ tmpl.path, copy.path ]);
}

VpnUrlHandler::Status evaluateStatus(NSURL *handler)
{
    if (!handler) {
        return VpnUrlHandler::Status::NotRegistered;
    }
    if (sameFile(handler, copyUrl())) {
        return isCopyUpToDate() ? VpnUrlHandler::Status::IsDefault : VpnUrlHandler::Status::NotRegistered;
    }
    if (isOurHelper(handler)) {
        return VpnUrlHandler::Status::NotRegistered;
    }
    NSString *handlerId = [NSBundle bundleWithURL:handler].bundleIdentifier;
    if ([handlerId isEqualToString:[NSBundle mainBundle].bundleIdentifier]) {
        return VpnUrlHandler::Status::NotRegistered;
    }
    return VpnUrlHandler::Status::OtherHandler;
}
} // namespace

VpnUrlHandler::Status VpnUrlHandler::status()
{
    @autoreleasepool {
        return evaluateStatus([[NSWorkspace sharedWorkspace] URLForApplicationToOpenURL:probeUrl()]);
    }
}

bool VpnUrlHandler::setAsDefault()
{
    @autoreleasepool {
        if (!ensureCopy()) {
            return false;
        }

        NSURL *copy = copyUrl();
        removeHelpers(copy);
        if (LSRegisterURL((__bridge CFURLRef)copy, true) != noErr) {
            return false;
        }

        [[NSWorkspace sharedWorkspace] setDefaultApplicationAtURL:copy
                                             toOpenURLsWithScheme:@"vpn"
                                                completionHandler:^(NSError *error) {
                                                }];
        return true;
    }
}

void VpnUrlHandler::unregister()
{
    @autoreleasepool {
        removeHelpers(nil);
    }
}

#endif
