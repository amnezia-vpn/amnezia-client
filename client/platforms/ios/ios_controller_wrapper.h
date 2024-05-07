#import <NetworkExtension/NetworkExtension.h>
#import <NetworkExtension/NETunnelProviderSession.h>
#import <Foundation/Foundation.h>
#include <UIKit/UIKit.h>
#include <Security/Security.h>

class IosController;

@interface IosControllerWrapper : NSObject {
    IosController *cppController;
}

- (instancetype)initWithCppController:(IosController *)controller;
- (void)vpnStatusDidChange:(NSNotification *)notification;
- (void)vpnConfigurationDidChange:(NSNotification *)notification;

@end

typedef void (^DocumentPickerClosedCallback)(NSString *path);

@interface DocumentPickerDelegate : NSObject <UIDocumentPickerDelegate>

@property (nonatomic, copy) DocumentPickerClosedCallback documentPickerClosedCallback;

@end
