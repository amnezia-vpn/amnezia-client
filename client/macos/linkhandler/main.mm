#import <AppKit/AppKit.h>

namespace {
NSString *const kMainBundleIdKey = @"AmneziaMainBundleIdentifier";
NSString *const kFallbackMainAppPath = @"/Applications/AmneziaVPN.app";
const NSTimeInterval kIdleTimeout = 5.0;

NSString *mainBundleId()
{
    return [NSBundle.mainBundle objectForInfoDictionaryKey:kMainBundleIdKey];
}

NSURL *mainAppUrl()
{
    NSString *bundleId = mainBundleId();
    NSArray<NSRunningApplication *> *running =
            bundleId.length ? [NSRunningApplication runningApplicationsWithBundleIdentifier:bundleId] : @[];
    if (running.firstObject.bundleURL) {
        return running.firstObject.bundleURL;
    }

    NSURL *url = bundleId.length ? [NSWorkspace.sharedWorkspace URLForApplicationWithBundleIdentifier:bundleId] : nil;
    if (!url && [NSFileManager.defaultManager fileExistsAtPath:kFallbackMainAppPath]) {
        url = [NSURL fileURLWithPath:kFallbackMainAppPath];
    }
    return url;
}

void launchMainExecutable(NSURL *appUrl, NSArray<NSURL *> *urls)
{
    NSURL *executable = [NSBundle bundleWithURL:appUrl].executableURL;
    if (!executable) {
        return;
    }
    NSMutableArray<NSString *> *args = [NSMutableArray array];
    for (NSURL *url in urls) {
        [args addObject:url.absoluteString];
    }
    [NSTask launchedTaskWithExecutableURL:executable arguments:args error:nil terminationHandler:nil];
}
} // namespace

@interface LinkHandlerDelegate : NSObject <NSApplicationDelegate>
@end

@implementation LinkHandlerDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    [self performSelector:@selector(quit) withObject:nil afterDelay:kIdleTimeout];
}

- (void)application:(NSApplication *)application openURLs:(NSArray<NSURL *> *)urls
{
    NSMutableArray<NSURL *> *vpnUrls = [NSMutableArray array];
    for (NSURL *url in urls) {
        if ([url.scheme caseInsensitiveCompare:@"vpn"] == NSOrderedSame) {
            [vpnUrls addObject:url];
        }
    }

    NSURL *appUrl = mainAppUrl();
    if (!vpnUrls.count || !appUrl) {
        [self quit];
        return;
    }

    NSString *bundleId = [NSBundle bundleWithURL:appUrl].bundleIdentifier ?: mainBundleId();
    if (@available(macOS 14.0, *)) {
        if (bundleId.length) {
            [NSApp yieldActivationToApplicationWithBundleIdentifier:bundleId];
        }
    }

    NSWorkspaceOpenConfiguration *configuration = [NSWorkspaceOpenConfiguration configuration];
    configuration.activates = YES;
    [NSWorkspace.sharedWorkspace openURLs:vpnUrls
                     withApplicationAtURL:appUrl
                            configuration:configuration
                        completionHandler:^(NSRunningApplication *app, NSError *error) {
                            if (error) {
                                launchMainExecutable(appUrl, vpnUrls);
                            }
                            dispatch_async(dispatch_get_main_queue(), ^{
                                [self quit];
                            });
                        }];
}

- (void)quit
{
    [NSApp terminate:nil];
}

@end

int main(int argc, const char *argv[])
{
    @autoreleasepool {
        NSApplication *application = NSApplication.sharedApplication;
        LinkHandlerDelegate *delegate = [LinkHandlerDelegate new];
        application.delegate = delegate;
        [application run];
    }
    return 0;
}
