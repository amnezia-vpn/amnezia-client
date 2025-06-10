/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "StoreKitController.h"
#import <StoreKit/StoreKit.h>

@interface StoreKitController () <SKProductsRequestDelegate, SKPaymentTransactionObserver>
@property (nonatomic, copy) void (^purchaseCompletion)(BOOL success, NSError *_Nullable error);
@property (nonatomic, copy) void (^restoreCompletion)(BOOL success, NSError *_Nullable error);
@property (nonatomic, strong) SKProductsRequest *productsRequest;
@end

@implementation StoreKitController

+ (instancetype)sharedInstance
{
    static dispatch_once_t onceToken;
    static StoreKitController *instance;
    dispatch_once(&onceToken, ^{
        instance = [[StoreKitController alloc] init];
    });
    return instance;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
    }
    return self;
}

- (void)dealloc
{
    [[SKPaymentQueue defaultQueue] removeTransactionObserver:self];
}

- (void)purchaseProduct:(NSString *)productIdentifier
             completion:(void (^)(BOOL success, NSError *_Nullable error))completion
{
    self.purchaseCompletion = completion;
    self.productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:[NSSet setWithObject:productIdentifier]];
    self.productsRequest.delegate = self;
    [self.productsRequest start];
}

- (void)restorePurchasesWithCompletion:(void (^)(BOOL success, NSError *_Nullable error))completion
{
    self.restoreCompletion = completion;
    [[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

#pragma mark - SKProductsRequestDelegate

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
    SKProduct *product = response.products.firstObject;
    if (!product) {
        if (self.purchaseCompletion) {
            NSError *error = [NSError errorWithDomain:@"StoreKitController"
                                                 code:0
                                             userInfo:@{ NSLocalizedDescriptionKey : @"Product not found" }];
            self.purchaseCompletion(NO, error);
            self.purchaseCompletion = nil;
        }
        return;
    }
    SKPayment *payment = [SKPayment paymentWithProduct:product];
    [[SKPaymentQueue defaultQueue] addPayment:payment];
    self.productsRequest = nil;
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
    if (self.purchaseCompletion) {
        self.purchaseCompletion(NO, error);
        self.purchaseCompletion = nil;
    }
    self.productsRequest = nil;
}

#pragma mark - SKPaymentTransactionObserver

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray<SKPaymentTransaction *> *)transactions
{
    for (SKPaymentTransaction *transaction in transactions) {
        switch (transaction.transactionState) {
        case SKPaymentTransactionStatePurchased:
            if (self.purchaseCompletion) {
                self.purchaseCompletion(YES, nil);
                self.purchaseCompletion = nil;
            }
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            self.productsRequest = nil;
            break;
        case SKPaymentTransactionStateFailed:
            if (self.purchaseCompletion) {
                self.purchaseCompletion(NO, transaction.error);
                self.purchaseCompletion = nil;
            }
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            self.productsRequest = nil;
            break;
        case SKPaymentTransactionStateRestored: [[SKPaymentQueue defaultQueue] finishTransaction:transaction]; break;
        case SKPaymentTransactionStatePurchasing:
        case SKPaymentTransactionStateDeferred: break;
        }
    }
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue
{
    if (self.restoreCompletion) {
        self.restoreCompletion(YES, nil);
        self.restoreCompletion = nil;
    }
}

- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error
{
    if (self.restoreCompletion) {
        self.restoreCompletion(NO, error);
        self.restoreCompletion = nil;
    }
}

@end
