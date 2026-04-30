/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef STOREKITCONTROLLER_H
#define STOREKITCONTROLLER_H

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

@class Product;
@class Transaction;
@class VerificationResult;

API_AVAILABLE(ios(15.0), macos(12.0))
@interface StoreKitController : NSObject

+ (instancetype)sharedInstance;

- (void)purchaseProduct:(NSString *)productIdentifier
             completion:(void (^)(BOOL success,
                                  NSString *_Nullable transactionId,
                                  NSString *_Nullable productId,
                                  NSString *_Nullable originalTransactionId,
                                  NSError *_Nullable error))completion;

- (void)restorePurchasesWithCompletion:(void (^)(BOOL success,
                                                 NSArray<NSDictionary *> *_Nullable restoredTransactions,
                                                 NSError *_Nullable error))completion;

// Fetch product information for a set of identifiers without initiating a purchase
- (void)fetchProductsWithIdentifiers:(NSSet<NSString *> *)productIdentifiers
                          completion:(void (^)(NSArray<NSDictionary *> *products,
                                               NSArray<NSString *> *invalidIdentifiers,
                                               NSError *_Nullable error))completion;

@end

#endif // STOREKITCONTROLLER_H
