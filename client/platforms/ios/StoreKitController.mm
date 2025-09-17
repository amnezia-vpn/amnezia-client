/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "StoreKitController.h"
#import <StoreKit/StoreKit.h>

API_AVAILABLE(ios(15.0), macos(12.0))
@interface StoreKitController () <SKProductsRequestDelegate, SKPaymentTransactionObserver>
@property (nonatomic, copy) void (^purchaseCompletion)(BOOL success,
                                                       NSString *_Nullable transactionId,
                                                       NSString *_Nullable productId,
                                                       NSString *_Nullable originalTransactionId,
                                                       NSError *_Nullable error);
@property (nonatomic, copy) void (^restoreCompletion)(BOOL success, NSError *_Nullable error);
@property (nonatomic, copy) void (^productsFetchCompletion)(NSArray<NSDictionary *> *products,
                                                            NSArray<NSString *> *invalidIdentifiers,
                                                            NSError *_Nullable error);
@property (nonatomic, strong) SKProductsRequest *productsRequest;
@end

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
                                  NSString *_Nullable originalTransactionId,
                                  NSError *_Nullable error))completion API_AVAILABLE(ios(15.0), macos(12.0))
{
    self.purchaseCompletion = completion;
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [self performPurchaseAsync:productIdentifier];
    });
}

- (void)performPurchaseAsync:(NSString *)productIdentifier API_AVAILABLE(ios(15.0), macos(12.0))
{
    dispatch_async(dispatch_get_main_queue(), ^{
        @try {
            SKProductsRequest *request = [[SKProductsRequest alloc] initWithProductIdentifiers:[NSSet setWithObject:productIdentifier]];
            request.delegate = self;
            [request start];
            
        } @catch (NSException *exception) {
            NSError *error = [NSError errorWithDomain:@"StoreKitController"
                                                 code:1
                                             userInfo:@{ NSLocalizedDescriptionKey : exception.reason ?: @"Purchase failed" }];
            if (self.purchaseCompletion) {
                self.purchaseCompletion(NO, nil, nil, nil, error);
                self.purchaseCompletion = nil;
            }
        }
    });
}

- (void)restorePurchasesWithCompletion:(void (^)(BOOL success, NSError *_Nullable error))completion API_AVAILABLE(ios(15.0), macos(12.0))
{
    self.restoreCompletion = completion;
    [[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

- (void)fetchProductsWithIdentifiers:(NSSet<NSString *> *)productIdentifiers
                          completion:(void (^)(NSArray<NSDictionary *> *products,
                                               NSArray<NSString *> *invalidIdentifiers,
                                               NSError *_Nullable error))completion API_AVAILABLE(ios(15.0), macos(12.0))
{
    self.productsFetchCompletion = completion;
    self.productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:productIdentifiers];
    self.productsRequest.delegate = self;
    [self.productsRequest start];
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
        NSMutableArray<NSDictionary *> *productDicts = [NSMutableArray array];
        for (SKProduct *p in response.products) {
            NSDictionary *productDict = @{
                @"productId": p.productIdentifier,
                @"title": p.localizedTitle,
                @"description": p.localizedDescription,
                @"price": p.price.stringValue,
                @"currencyCode": [p.priceLocale objectForKey:NSLocaleCurrencyCode] ?: @""
            };
            [productDicts addObject:productDict];
        }
        
        self.productsFetchCompletion(productDicts, response.invalidProductIdentifiers, nil);
        self.productsFetchCompletion = nil;
        self.productsRequest = nil;
        return;
    }
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error
{
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
            NSString *originalTransactionId = transaction.originalTransaction.transactionIdentifier ?: transaction.transactionIdentifier;
            
            if (self.purchaseCompletion) {
                self.purchaseCompletion(YES,
                                       transaction.transactionIdentifier,
                                       transaction.payment.productIdentifier,
                                       originalTransactionId,
                                       nil);
                self.purchaseCompletion = nil;
            }
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
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
            break;
        case SKPaymentTransactionStateRestored: 
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction]; 
            break;
        case SKPaymentTransactionStatePurchasing:
        case SKPaymentTransactionStateDeferred: 
            break;
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
