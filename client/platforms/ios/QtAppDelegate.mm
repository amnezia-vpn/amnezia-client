#import "QtAppDelegate.h"
#import "ios_controller.h"

#include <QFile>


@implementation QIOSApplicationDelegate (AmneziaVPNDelegate)

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    [application setMinimumBackgroundFetchInterval: UIApplicationBackgroundFetchIntervalMinimum];
    // Override point for customization after application launch.
    NSLog(@"Application didFinishLaunchingWithOptions");
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
    if (url.fileURL) {
        QString filePath(url.path.UTF8String);
        if (filePath.isEmpty()) return NO;

        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
            NSLog(@"Application openURL: %@", url);

            if (filePath.contains("backup")) {
                IosController::Instance()->importBackupFromOutside(filePath);
            } else {
                QFile file(filePath);
                bool isOpenFile = file.open(QIODevice::ReadOnly);
                QByteArray data = file.readAll();

                IosController::Instance()->importConfigFromOutside(QString(data));
            }
        });

        return YES;
    }
    return NO;
}

@end
