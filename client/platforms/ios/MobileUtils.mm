#include "MobileUtils.h"

#include <UIKit/UIKit.h>
#include <Security/Security.h>

#include <QDebug>
#include <QEventLoop>

static UIViewController* getViewController() {
    NSArray *windows = [[UIApplication sharedApplication]windows];
    for (UIWindow *window in windows) {
        if (window.isKeyWindow) {
            return window.rootViewController;
        }
    }
    return nil;
}

void MobileUtils::shareText(const QStringList& filesToSend) {
    NSMutableArray *sharingItems = [NSMutableArray new];

    for (int i = 0; i < filesToSend.size(); i++) {
        NSURL *logFileUrl = [[NSURL alloc] initFileURLWithPath:filesToSend[i].toNSString()];
        [sharingItems addObject:logFileUrl];
    }

    UIViewController *qtController = getViewController();
    if (!qtController) return;

    UIActivityViewController *activityController = [[UIActivityViewController alloc] initWithActivityItems:sharingItems applicationActivities:nil];

    [qtController presentViewController:activityController animated:YES completion:nil];
    UIPopoverPresentationController *popController = activityController.popoverPresentationController;
    if (popController) {
        popController.sourceView = qtController.view;
    }
}

typedef void (^FileSelectionCallback)(NSString *fileContent);

@interface MyFilePickerDelegate : NSObject <UIDocumentPickerDelegate>

@property (nonatomic, copy) FileSelectionCallback fileSelectionCallback;

@end

@implementation MyFilePickerDelegate

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    for (NSURL *url in urls) {
        if (self.fileSelectionCallback) {
            self.fileSelectionCallback([url path]);
        }
    }
}
    
- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
    if (self.fileSelectionCallback) {
        self.fileSelectionCallback(nil);
    }
}

@end

QString MobileUtils::openFile() {
    UIDocumentPickerViewController *documentPicker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[@"public.item"] inMode:UIDocumentPickerModeOpen];

    MyFilePickerDelegate *filePickerDelegate = [[MyFilePickerDelegate alloc] init];
    documentPicker.delegate = filePickerDelegate;

    UIViewController *qtController = getViewController();
    if (!qtController) return;

    [qtController presentViewController:documentPicker animated:YES completion:nil];
    
    __block QString path1;

    filePickerDelegate.fileSelectionCallback = ^(NSString *filePath) {
        if (filePath) {
            path1 = QString::fromUtf8(filePath.UTF8String);
        } else {
            path1 = QString("");
        }
        emit finished();
    };

    QEventLoop wait1;
    QObject::connect(this, &MobileUtils::finished, &wait1, &QEventLoop::quit);
    wait1.exec();
    
    return path1;
}
