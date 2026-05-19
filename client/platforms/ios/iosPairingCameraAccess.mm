#include "platforms/ios/iosPairingCameraAccess.h"

#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>

bool amneziaIosPairingCameraAccessGranted()
{
    const AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    return status == AVAuthorizationStatusAuthorized;
}

void amneziaIosRequestPairingCameraAccess(const std::function<void(bool)> &onDone)
{
    const AVAuthorizationStatus status = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if (status == AVAuthorizationStatusAuthorized) {
        onDone(true);
        return;
    }
    if (status == AVAuthorizationStatusDenied || status == AVAuthorizationStatusRestricted) {
        onDone(false);
        return;
    }
    [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo
                             completionHandler:^(BOOL granted) {
                                 dispatch_async(dispatch_get_main_queue(), ^{
                                     onDone(static_cast<bool>(granted));
                                 });
                             }];
}

void amneziaIosOpenApplicationSettings()
{
    NSURL *url = [NSURL URLWithString:UIApplicationOpenSettingsURLString];
    if (url != nil) {
        [[UIApplication sharedApplication] openURL:url options:@{} completionHandler:nil];
    }
}
