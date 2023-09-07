#include "MobileUtils.h"

#include <UIKit/UIKit.h>
#include <Security/Security.h>

#include <QDebug>

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

@interface MyFilePickerDelegate : NSObject <UIDocumentPickerDelegate>
@end

@implementation MyFilePickerDelegate

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    for (NSURL *url in urls) {
        NSString *filePath = [url path];
        
        NSData *fileData = [NSData dataWithContentsOfFile:filePath];
        NSString *fileContent = [[NSString alloc] initWithData:fileData encoding:NSUTF8StringEncoding];
        NSLog(@"Содержимое файла: %@", fileContent);
    }
}

@end

void MobileUtils::openFile() {
    UIDocumentPickerViewController *documentPicker = [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[@"public.item"] inMode:UIDocumentPickerModeOpen];

    MyFilePickerDelegate *filePickerDelegate = [[MyFilePickerDelegate alloc] init];
    documentPicker.delegate = filePickerDelegate;

    UIViewController *qtController = getViewController();
    if (!qtController) return;

    [qtController presentViewController:documentPicker animated:YES completion:nil];
}
