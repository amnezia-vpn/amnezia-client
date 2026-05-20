#import "QtAppDelegate.h"
#import "AmneziaOpenUrlImport.h"

@implementation QIOSApplicationDelegate (AmneziaVPNDelegate)
#if !MACOS_NE
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    [application setMinimumBackgroundFetchInterval: UIApplicationBackgroundFetchIntervalMinimum];
    // Override point for customization after application launch.
    NSLog(@"Application didFinishLaunchingWithOptions");
    NSURL *launchUrl = launchOptions[UIApplicationLaunchOptionsURLKey];
    if (launchUrl) {
        AmneziaHandleOpenUrl(launchUrl);
    }
    return YES;
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    NSLog(@"In the background");
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
    NSLog(@"In the foreground");
}

-(void)application:(UIApplication *)application performFetchWithCompletionHandler:(void (^)(UIBackgroundFetchResult))completionHandler {
    // We will add content here soon.
    NSLog(@"In the completionHandler");
}

- (BOOL)application:(UIApplication *)app
            openURL:(NSURL *)url
            options:(NSDictionary<UIApplicationOpenURLOptionsKey, id> *)options {
    NSLog(@"Application openURL: %@", url);
    AmneziaHandleOpenUrl(url);

    NSString *scheme = url.scheme ? [url.scheme lowercaseString] : @"";
    if ([scheme isEqualToString:@"vpn"] || url.fileURL) {
        return YES;
    }
    return NO;
}
#endif
@end
