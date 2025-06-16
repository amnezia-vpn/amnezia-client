/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef STOREKITCONTROLLER_H
#define STOREKITCONTROLLER_H

#import <Foundation/Foundation.h>

@interface StoreKitController : NSObject

+ (instancetype)sharedInstance;

- (void)purchaseProduct:(NSString *)productIdentifier
             completion:(void (^)(BOOL success,
                                  NSString *_Nullable transactionId,
                                  NSString *_Nullable productId,
                                  NSError *_Nullable error))completion;

- (void)restorePurchasesWithCompletion:(void (^)(BOOL success, NSError *_Nullable error))completion;

@end

#endif // STOREKITCONTROLLER_H
