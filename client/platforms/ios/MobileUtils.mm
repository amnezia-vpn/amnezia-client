#include "MobileUtils.h"

#include <UIKit/UIKit.h>
#include <Security/Security.h>

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

MobileUtils::MobileUtils(QObject *parent) : QObject(parent) {
    
}

bool MobileUtils::shareText(const QStringList& filesToSend) {
    NSMutableArray *sharingItems = [NSMutableArray new];

    for (int i = 0; i < filesToSend.size(); i++) {
        NSURL *logFileUrl = [[NSURL alloc] initFileURLWithPath:filesToSend[i].toNSString()];
        [sharingItems addObject:logFileUrl];
    }

    UIViewController *qtController = getViewController();
    if (!qtController) return;

    UIActivityViewController *activityController = [[UIActivityViewController alloc] initWithActivityItems:sharingItems applicationActivities:nil];
    
    __block bool isAccepted = false;
    
    [activityController setCompletionWithItemsHandler:^(NSString *activityType, BOOL completed, NSArray *returnedItems, NSError *activityError) {
        isAccepted = completed;
        emit finished();
    }];

    [qtController presentViewController:activityController animated:YES completion:nil];
    UIPopoverPresentationController *popController = activityController.popoverPresentationController;
    if (popController) {
        popController.sourceView = qtController.view;
    }
    
    QEventLoop wait;
    QObject::connect(this, &MobileUtils::finished, &wait, &QEventLoop::quit);
    wait.exec();
    
    return isAccepted;
}

typedef void (^DocumentPickerClosedCallback)(NSString *path);

@interface DocumentPickerDelegate : NSObject <UIDocumentPickerDelegate>

@property (nonatomic, copy) DocumentPickerClosedCallback documentPickerClosedCallback;

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

QString MobileUtils::openFile() {
    UIDocumentPickerViewController *documentPicker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[@"public.item"] inMode:UIDocumentPickerModeOpen];

    DocumentPickerDelegate *documentPickerDelegate = [[DocumentPickerDelegate alloc] init];
    documentPicker.delegate = documentPickerDelegate;

    UIViewController *qtController = getViewController();
    if (!qtController) return;

    [qtController presentViewController:documentPicker animated:YES completion:nil];
    
    __block QString filePath;

    documentPickerDelegate.documentPickerClosedCallback = ^(NSString *path) {
        if (path) {
            filePath = QString::fromUtf8(path.UTF8String);
        } else {
            filePath = QString();
        }
        emit finished();
    };

    QEventLoop wait;
    QObject::connect(this, &MobileUtils::finished, &wait, &QEventLoop::quit);
    wait.exec();
    
    return filePath;
}
