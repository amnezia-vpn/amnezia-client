#include "macos_ne_vpn_notification.h"

#include <QtGlobal>
#include <QString>

#import <Foundation/Foundation.h>
#import <UserNotifications/UserNotifications.h>

namespace {

@interface MacosNeVpnNotificationDelegate : NSObject <UNUserNotificationCenterDelegate>
@end

@implementation MacosNeVpnNotificationDelegate

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
       willPresentNotification:(UNNotification *)notification
         withCompletionHandler:(void (^)(UNNotificationPresentationOptions options))completionHandler
{
    Q_UNUSED(center)
    Q_UNUSED(notification)
    completionHandler(UNNotificationPresentationOptionList | UNNotificationPresentationOptionBanner);
}

- (void)userNotificationCenter:(UNUserNotificationCenter *)center
    didReceiveNotificationResponse:(UNNotificationResponse *)response
             withCompletionHandler:(void (^)(void))completionHandler
{
    Q_UNUSED(center)
    Q_UNUSED(response)
    completionHandler();
}

@end

MacosNeVpnNotificationDelegate *delegateInstance()
{
    static MacosNeVpnNotificationDelegate *d;
    static dispatch_once_t once;
    dispatch_once(&once, ^{
      d = [[MacosNeVpnNotificationDelegate alloc] init];
    });
    return d;
}

void ensureNotificationCenterSetup()
{
    static dispatch_once_t once;
    dispatch_once(&once, ^{
      UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
      [center requestAuthorizationWithOptions:(UNAuthorizationOptionSound | UNAuthorizationOptionAlert |
                                               UNAuthorizationOptionBadge)
                            completionHandler:^(BOOL granted, NSError *_Nullable error) {
                              Q_UNUSED(granted);
                              if (!error) {
                                  center.delegate = delegateInstance();
                              }
                            }];
    });
}

} // namespace

void macosNePostVpnStateNotification(const QString &title, const QString &message)
{
    ensureNotificationCenterSetup();

    UNMutableNotificationContent *content = [[UNMutableNotificationContent alloc] init];
    content.title = title.toNSString();
    content.body = message.toNSString();
    content.sound = nil;

    NSTimeInterval delay = 0.1;
    UNTimeIntervalNotificationTrigger *trigger =
        [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:delay repeats:NO];

    NSString *identifier =
        [NSString stringWithFormat:@"amneziavpn.vpnstate.%lld", (long long)([[NSDate date] timeIntervalSince1970] * 1000.0)];

    UNNotificationRequest *request = [UNNotificationRequest requestWithIdentifier:identifier
                                                                          content:content
                                                                          trigger:trigger];

    UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
    [center addNotificationRequest:request
             withCompletionHandler:^(NSError *_Nullable error) {
               if (error) {
                   NSLog(@"macosNePostVpnStateNotification failed: %@", error);
               }
             }];
}
