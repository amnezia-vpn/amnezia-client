/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "StoreKitController.h"
#import <StoreKit/StoreKit.h>

@interface StoreKitController () <SKProductsRequestDelegate, SKPaymentTransactionObserver>
@property (nonatomic, copy) void (^purchaseCompletion)(BOOL success,
                                                       NSString *_Nullable transactionId,
                                                       NSString *_Nullable productId,
                                                       NSString *_Nullable receiptBase64,
                                                       NSError *_Nullable error);
@property (nonatomic, copy) void (^restoreCompletion)(BOOL success, NSError *_Nullable error);
@property (nonatomic, copy) void (^productsFetchCompletion)(NSArray<SKProduct *> *products,
                                                            NSArray<NSString *> *invalidIdentifiers,
                                                            NSError *_Nullable error);
@property (nonatomic, strong) SKProductsRequest *productsRequest;
@property (nonatomic, strong) SKReceiptRefreshRequest *receiptRefreshRequest;
@property (nonatomic, copy) NSString *pendingTransactionId;
@property (nonatomic, copy) NSString *pendingProductId;
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
             completion:(void (^)(BOOL success,
                                  NSString *_Nullable transactionId,
                                  NSString *_Nullable productId,
                                  NSString *_Nullable receiptBase64,
                                  NSError *_Nullable error))completion
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

- (void)fetchProductsWithIdentifiers:(NSSet<NSString *> *)productIdentifiers
                          completion:(void (^)(NSArray<SKProduct *> *products,
                                               NSArray<NSString *> *invalidIdentifiers,
                                               NSError *_Nullable error))completion
{
    self.productsFetchCompletion = completion;
    self.productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:productIdentifiers];
    self.productsRequest.delegate = self;
    [self.productsRequest start];
}

// Helper to read the Base64-encoded app receipt from bundle
- (NSString *)base64AppReceipt
{
    NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
    if (!receiptURL) {
        return nil;
    }
    NSData *receiptData = [NSData dataWithContentsOfURL:receiptURL];
    if (!receiptData || receiptData.length == 0) {
        return nil;
    }
    return [receiptData base64EncodedStringWithOptions:0];
}

// Start a receipt refresh request to obtain or update the app receipt
- (void)startReceiptRefresh
{
    self.receiptRefreshRequest = [[SKReceiptRefreshRequest alloc] initWithReceiptProperties:nil];
    self.receiptRefreshRequest.delegate = self;
    [self.receiptRefreshRequest start];
}

#pragma mark - SKProductsRequestDelegate / SKRequestDelegate

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
    if (self.purchaseCompletion) {
        SKProduct *product = response.products.firstObject;
        if (!product) {
            NSError *error = [NSError errorWithDomain:@"StoreKitController"
                                                 code:0
                                             userInfo:@{ NSLocalizedDescriptionKey : @"Product not found" }];
            self.purchaseCompletion(NO, nil, nil, nil, error);
            self.purchaseCompletion = nil;
            self.productsRequest = nil;
            return;
        }
        SKPayment *payment = [SKPayment paymentWithProduct:product];
        [[SKPaymentQueue defaultQueue] addPayment:payment];
        self.productsRequest = nil;
        return;
    }

    if (self.productsFetchCompletion) {
        self.productsFetchCompletion(response.products, response.invalidProductIdentifiers, nil);
        self.productsFetchCompletion = nil;
        self.productsRequest = nil;
        return;
    }
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
    // Distinguish between product fetch vs. receipt refresh failure
    if (request == self.receiptRefreshRequest) {
        // Receipt refresh failed; if we still have a pending purchase, complete without receipt
        if (self.purchaseCompletion) {
            self.purchaseCompletion(YES, self.pendingTransactionId, self.pendingProductId, nil, nil);
            self.purchaseCompletion = nil;
        }
        self.pendingTransactionId = nil;
        self.pendingProductId = nil;
        self.receiptRefreshRequest = nil;
        return;
    }

    if (self.purchaseCompletion) {
        self.purchaseCompletion(NO, nil, nil, nil, error);
        self.purchaseCompletion = nil;
    }
    if (self.productsFetchCompletion) {
        self.productsFetchCompletion(@[], @[], error);
        self.productsFetchCompletion = nil;
    }
    self.productsRequest = nil;
}

#pragma mark - SKPaymentTransactionObserver

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray<SKPaymentTransaction *> *)transactions
{
    for (SKPaymentTransaction *transaction in transactions) {
        switch (transaction.transactionState) {
        case SKPaymentTransactionStatePurchased: {
            // On purchase success, try to attach the Base64 app receipt
            NSString *receipt = [self base64AppReceipt];
            if (receipt.length > 0) {
                if (self.purchaseCompletion) {
                    self.purchaseCompletion(YES,
                                           transaction.transactionIdentifier,
                                           transaction.payment.productIdentifier,
                                           receipt,
                                           nil);
                    self.purchaseCompletion = nil;
                }
            } else {
                // No receipt found yet: start a refresh and defer completion
                self.pendingTransactionId = transaction.transactionIdentifier;
                self.pendingProductId = transaction.payment.productIdentifier;
                [self startReceiptRefresh];
            }
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            self.productsRequest = nil;
            break;
        }
        case SKPaymentTransactionStateFailed:
            if (self.purchaseCompletion) {
                self.purchaseCompletion(NO,
                                       transaction.transactionIdentifier,
                                       transaction.payment.productIdentifier,
                                       nil,
                                       transaction.error);
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

- (void)requestDidFinish:(SKRequest *)request
{
    if (request == self.receiptRefreshRequest) {
        NSString *receipt = [self base64AppReceipt];
        if (self.purchaseCompletion) {
            self.purchaseCompletion(YES, self.pendingTransactionId, self.pendingProductId, receipt, nil);
            self.purchaseCompletion = nil;
        }
        self.pendingTransactionId = nil;
        self.pendingProductId = nil;
        self.receiptRefreshRequest = nil;
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
