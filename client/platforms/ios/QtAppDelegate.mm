#import "QtAppDelegate.h"
#import "ios_controller.h"
#import <StoreKit/StoreKit.h>

#include <QFile>

namespace {
constexpr NSInteger kReviewRequestOpenInterval = 20;
NSString *const kAppOpenCountKey = @"AmneziaVPN.AppOpenCount";
BOOL gShouldRequestReviewOnBecomeActive = NO;
id gSceneWillEnterForegroundObserver = nil;
id gSceneDidActivateObserver = nil;

void scheduleAppStoreReviewIfNeededOnOpen()
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    const NSInteger openCount = [defaults integerForKey:kAppOpenCountKey] + 1;
    [defaults setInteger:openCount forKey:kAppOpenCountKey];

    gShouldRequestReviewOnBecomeActive =
        (openCount > 0 && openCount % kReviewRequestOpenInterval == 0);

    NSLog(@"[Review] scene open count=%ld interval=%ld scheduled=%@",
          (long)openCount,
          (long)kReviewRequestOpenInterval,
          gShouldRequestReviewOnBecomeActive ? @"YES" : @"NO");
}

void requestAppStoreReviewForScene(UIScene *scene)
{
    if (@available(iOS 14.0, *)) {
        if ([scene isKindOfClass:[UIWindowScene class]] &&
            scene.activationState == UISceneActivationStateForegroundActive) {
            [SKStoreReviewController requestReviewInScene:(UIWindowScene *)scene];
            NSLog(@"[Review] requestReviewInScene invoked");
            return;
        }
    }

    if (@available(iOS 10.3, *)) {
        [SKStoreReviewController requestReview];
        NSLog(@"[Review] requestReview fallback invoked");
    }
}

void setupSceneReviewObservers()
{
    if (gSceneWillEnterForegroundObserver || gSceneDidActivateObserver) {
        return;
    }

    NSNotificationCenter *center = [NSNotificationCenter defaultCenter];

    if (@available(iOS 13.0, *)) {
        gSceneWillEnterForegroundObserver =
            [center addObserverForName:UISceneWillEnterForegroundNotification
                                object:nil
                                 queue:[NSOperationQueue mainQueue]
                            usingBlock:^(__unused NSNotification *note) {
            scheduleAppStoreReviewIfNeededOnOpen();
        }];

        gSceneDidActivateObserver =
            [center addObserverForName:UISceneDidActivateNotification
                                object:nil
                                 queue:[NSOperationQueue mainQueue]
                            usingBlock:^(NSNotification *note) {
            if (!gShouldRequestReviewOnBecomeActive) {
                return;
            }

            gShouldRequestReviewOnBecomeActive = NO;
            UIScene *scene = [note.object isKindOfClass:[UIScene class]] ? (UIScene *)note.object : nil;
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                requestAppStoreReviewForScene(scene);
            });
        }];
    }
}
} // namespace


@implementation QIOSApplicationDelegate (AmneziaVPNDelegate)
#if !MACOS_NE
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    [application setMinimumBackgroundFetchInterval: UIApplicationBackgroundFetchIntervalMinimum];
    setupSceneReviewObservers();
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
#endif
@end
