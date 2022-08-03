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

bool deleteFromKeychain(const QString& tag) {
    NSData* nsTag = [tag.toNSString() dataUsingEncoding:NSUTF8StringEncoding];

    NSDictionary *deleteQuery = @{ (id)kSecAttrAccount: nsTag,
                                   (id)kSecClass: (id)kSecClassGenericPassword,
                                 };

    OSStatus status = SecItemDelete((__bridge CFDictionaryRef)deleteQuery);
    if (status != errSecSuccess) {
        qDebug() << "Error deleteFromKeychain" << status;
        return false;
    } else {
        qDebug() << "OK deleteFromKeychain";
        return true;
    }
}

void MobileUtils::writeToKeychain(const QString& tag, const QString& value) {
    deleteFromKeychain(tag);

    NSData* nsTag = [tag.toNSString() dataUsingEncoding:NSUTF8StringEncoding];
    NSData* nsValue = [value.toNSString() dataUsingEncoding:NSUTF8StringEncoding];

    NSDictionary* addQuery = @{ (id)kSecAttrAccount: nsTag,
                                (id)kSecClass: (id)kSecClassGenericPassword,
                                (id)kSecValueData: nsValue,
                              };

    OSStatus status = SecItemAdd((__bridge CFDictionaryRef)addQuery, NULL);
    if (status != errSecSuccess) {
        qDebug() << "Error writeToKeychain" << status;
    } else {
        qDebug() << "OK writeToKeychain";
    }
}

QString MobileUtils::readFromKeychain(const QString& tag) {
    NSData* nsTag = [tag.toNSString() dataUsingEncoding:NSUTF8StringEncoding];
    NSData* nsValue = NULL;

    NSDictionary *getQuery = @{ (id)kSecAttrAccount: nsTag,
                                (id)kSecClass: (id)kSecClassGenericPassword,
                                (id)kSecMatchLimit: (id)kSecMatchLimitOne,
                                (id)kSecReturnData: @YES,
                              };
    
    OSStatus status = SecItemCopyMatching((__bridge CFDictionaryRef)getQuery,
                                          (CFTypeRef *)&nsValue);
    if (status != errSecSuccess) {
        qDebug() << "Error readFromKeychain" << status;
    } else {
        qDebug() << "OK readFromKeychain" << nsValue;
    }

    QString result;
    if (nsValue) {
        result = QByteArray::fromNSData(nsValue);
        CFRelease(nsValue);
    }
    
    return result;
}
