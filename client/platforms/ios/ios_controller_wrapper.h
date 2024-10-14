#import <NetworkExtension/NetworkExtension.h>
#import <NetworkExtension/NETunnelProviderSession.h>
#import <Foundation/Foundation.h>

#if !MACOS_NE
#include <UIKit/UIKit.h>
#endif

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
#if !MACOS_NE
@interface DocumentPickerDelegate : NSObject <UIDocumentPickerDelegate>

@property (nonatomic, copy) DocumentPickerClosedCallback documentPickerClosedCallback;

@end
#endif
