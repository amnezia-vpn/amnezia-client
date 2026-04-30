/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "StoreKitController.h"
#import <StoreKit/StoreKit.h>
#import <AmneziaVPN-Swift.h>

#include <QtCore/QDebug>
#include <QtCore/QString>

namespace
{
QString toQString(NSString *value)
{
    return QString::fromUtf8((value ?: @"").UTF8String);
}
}

API_AVAILABLE(ios(15.0), macos(12.0))
@implementation StoreKitController

+ (instancetype)sharedInstance
{
    static dispatch_once_t onceToken;
    static StoreKitController *instance;
    dispatch_once(&onceToken, ^{
        if (@available(iOS 15.0, macOS 12.0, *)) {
            instance = [[StoreKitController alloc] init];
        }
    });
    return instance;
}

- (instancetype)init API_AVAILABLE(ios(15.0), macos(12.0))
{
    self = [super init];
    return self;
}

- (void)purchaseProduct:(NSString *)productIdentifier
             completion:(void (^)(BOOL success,
                                  NSString *_Nullable transactionId,
                                  NSString *_Nullable productId,
                                  NSString *_Nullable originalTransactionId,
                                  NSError *_Nullable error))completion API_AVAILABLE(ios(15.0), macos(12.0))
{
    qInfo().noquote() << "[IAP][StoreKit2] Starting purchase for" << QString::fromUtf8(productIdentifier.UTF8String);
    [[StoreKit2Helper shared] purchaseProductWithProductIdentifier:productIdentifier
                                                      completion:^(BOOL success,
                                                                   NSString *transactionId,
                                                                   NSString *productId,
                                                                   NSString *originalTransactionId,
                                                                   NSError *error) {
        if (success) {
            qInfo().noquote() << "[IAP][StoreKit2] Purchase success. transactionId =" << toQString(transactionId)
                              << "originalTransactionId =" << toQString(originalTransactionId) << "productId =" << toQString(productId);
        } else if (error) {
            qWarning().noquote() << "[IAP][StoreKit2] Purchase failed:" << toQString(error.localizedDescription);
        }
        if (completion) {
            completion(success, transactionId, productId, originalTransactionId, error);
        }
    }];
}

- (void)restorePurchasesWithCompletion:(void (^)(BOOL success,
                                                 NSArray<NSDictionary *> *_Nullable restoredTransactions,
                                                 NSError *_Nullable error))completion API_AVAILABLE(ios(15.0), macos(12.0))
{
    [[StoreKit2Helper shared] fetchCurrentEntitlementsWithCompletion:^(BOOL success,
                                                                      NSArray<NSDictionary *> *entitlements,
                                                                      NSError *error) {
        if (success) {
            qInfo().noquote() << "[IAP][StoreKit2] currentEntitlements returned"
                              << (int)(entitlements ? entitlements.count : 0) << "active entitlements";
            for (NSDictionary *entitlement in entitlements) {
                qInfo().noquote() << "[IAP][StoreKit2] Active entitlement:"
                                  << "transactionId=" << toQString(entitlement[@"transactionId"])
                                  << "originalTransactionId=" << toQString(entitlement[@"originalTransactionId"])
                                  << "productId=" << toQString(entitlement[@"productId"]);
            }
        } else {
            qWarning().noquote() << "[IAP][StoreKit2] fetchCurrentEntitlements failed:" << toQString(error.localizedDescription);
        }
        if (completion) {
            completion(success, entitlements, error);
        }
    }];
}

- (void)fetchProductsWithIdentifiers:(NSSet<NSString *> *)productIdentifiers
                          completion:(void (^)(NSArray<NSDictionary *> *products,
                                               NSArray<NSString *> *invalidIdentifiers,
                                               NSError *_Nullable error))completion API_AVAILABLE(ios(15.0), macos(12.0))
{
    [[StoreKit2Helper shared] fetchProductsWithIdentifiers:productIdentifiers
                                               completion:^(NSArray<NSDictionary *> *products,
                                                            NSArray<NSString *> *invalidIdentifiers,
                                                            NSError *error) {
        if (!error) {
            for (NSDictionary *productInfo in products) {
                qInfo().noquote() << "[IAP][StoreKit2] Fetched product info" << toQString(productInfo[@"productId"])
                                  << "price=" << toQString(productInfo[@"price"])
                                  << "currency=" << toQString(productInfo[@"currencyCode"]);
            }
        }
        if (completion) {
            completion(products ?: @[], invalidIdentifiers ?: @[], error);
        }
    }];
}

@end
