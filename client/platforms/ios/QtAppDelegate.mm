#import "QtAppDelegate.h"
#import "ios_controller.h"

#include <QFile>

UIView *_screen;

@implementation QIOSApplicationDelegate (AmneziaVPNDelegate)

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    [application setMinimumBackgroundFetchInterval: UIApplicationBackgroundFetchIntervalMinimum];
    // Override point for customization after application launch.
    NSLog(@"Application didFinishLaunchingWithOptions");
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
    _screen = [UIScreen.mainScreen snapshotViewAfterScreenUpdates: false];
    UIBlurEffect *blurEffect = [UIBlurEffect effectWithStyle: UIBlurEffectStyleDark];
    UIVisualEffectView *blurBackground = [[UIVisualEffectView alloc] initWithEffect: blurEffect];
    [_screen addSubview: blurBackground];
    blurBackground.frame = _screen.frame;
    UIWindow *_window = UIApplication.sharedApplication.keyWindow;
    [_window addSubview: _screen];
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

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    [_screen removeFromSuperview];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
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
