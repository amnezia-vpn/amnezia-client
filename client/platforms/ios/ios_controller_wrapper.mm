#import "ios_controller_wrapper.h"
#include "ios_controller.h"

@implementation IosControllerWrapper

- (instancetype)initWithCppController:(IosController *)controller {
    self = [super init];
    if (self) {
        cppController = controller;
    }
    return self;
}

- (void)vpnStatusDidChange:(NSNotification *)notification {

    NETunnelProviderSession *session = (NETunnelProviderSession *)notification.object;

    if (session ) {
        cppController->vpnStatusDidChange(session);
    }
}

- (void) vpnConfigurationDidChange:(NSNotification *)notification {
//    cppController->vpnStatusDidChange(notification);
}

@end

@implementation DocumentPickerDelegate 

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    for (NSURL *url in urls) {
        if (self.documentPickerClosedCallback) {
            self.documentPickerClosedCallback([url path]);
        }
    }
}
    
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
    if (self.documentPickerClosedCallback) {
        self.documentPickerClosedCallback(nil);
    }
}

@end