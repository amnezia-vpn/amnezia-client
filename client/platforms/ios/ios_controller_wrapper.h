#import <NetworkExtension/NetworkExtension.h>
#import <NetworkExtension/NETunnelProviderSession.h>
#import <Foundation/Foundation.h>

class IosController;

@interface IosControllerWrapper : NSObject {
    IosController *cppController;
}

- (instancetype)initWithCppController:(IosController *)controller;
- (void)vpnStatusDidChange:(NSNotification *)notification;
- (void)vpnConfigurationDidChange:(NSNotification *)notification;

@end
