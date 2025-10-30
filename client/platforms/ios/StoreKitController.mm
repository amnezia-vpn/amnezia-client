/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#import "StoreKitController.h"
#import <StoreKit/StoreKit.h>

#include <QtCore/QDebug>
#include <QtCore/QString>

API_AVAILABLE(ios(15.0), macos(12.0))
@interface StoreKitController () <SKProductsRequestDelegate, SKPaymentTransactionObserver>
@property (nonatomic, copy) void (^purchaseCompletion)(BOOL success,
                                                       NSString *_Nullable transactionId,
                                                       NSString *_Nullable productId,
                                                       NSString *_Nullable originalTransactionId,
                                                       NSError *_Nullable error);
@property (nonatomic, copy) void (^restoreCompletion)(BOOL success,
                                                      NSArray<NSDictionary *> *_Nullable restoredTransactions,
                                                      NSError *_Nullable error);
@property (nonatomic, copy) void (^productsFetchCompletion)(NSArray<NSDictionary *> *products,
                                                            NSArray<NSString *> *invalidIdentifiers,
                                                            NSError *_Nullable error);
@property (nonatomic, strong) SKProductsRequest *productsRequest;
@property (nonatomic, strong) NSMutableArray<NSDictionary *> *restoredTransactions;
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
    
    qInfo().noquote() << "[IAP][StoreKit] Starting purchase for" << QString::fromUtf8(productIdentifier.UTF8String);
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

- (void)restorePurchasesWithCompletion:(void (^)(BOOL success,
                                                 NSArray<NSDictionary *> *_Nullable restoredTransactions,
                                                 NSError *_Nullable error))completion API_AVAILABLE(ios(15.0), macos(12.0))
{
    self.restoreCompletion = completion;
    self.restoredTransactions = [NSMutableArray array];
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
        NSString *currencyCode = [product.priceLocale objectForKey:NSLocaleCurrencyCode] ?: @"";
        NSString *priceString = [product.price stringValue] ?: @"";
        qInfo().noquote() << "[IAP][StoreKit] Received product" << QString::fromUtf8(product.productIdentifier.UTF8String)
                          << "price=" << QString::fromUtf8(priceString.UTF8String)
                          << "currency=" << QString::fromUtf8(currencyCode.UTF8String);
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
            NSString *productCurrency = [p.priceLocale objectForKey:NSLocaleCurrencyCode] ?: @"";
            NSString *productPrice = [p.price stringValue] ?: @"";
            qInfo().noquote() << "[IAP][StoreKit] Fetched product info" << QString::fromUtf8(p.productIdentifier.UTF8String)
                              << "price=" << QString::fromUtf8(productPrice.UTF8String)
                              << "currency=" << QString::fromUtf8(productCurrency.UTF8String);
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
            qInfo().noquote() << "[IAP][StoreKit] Transaction purchased" << QString::fromUtf8(transaction.transactionIdentifier.UTF8String)
                              << "original=" << QString::fromUtf8((originalTransactionId ?: @"").UTF8String)
                              << "product=" << QString::fromUtf8(transaction.payment.productIdentifier.UTF8String);
            
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
            qInfo().noquote() << "[IAP][StoreKit] Transaction failed" << QString::fromUtf8(transaction.transactionIdentifier.UTF8String)
                              << "product=" << QString::fromUtf8(transaction.payment.productIdentifier.UTF8String)
                              << "error=" << QString::fromUtf8(transaction.error.localizedDescription.UTF8String);
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
        case SKPaymentTransactionStateRestored: {
            if (self.restoreCompletion) {
                NSString *transactionId = transaction.transactionIdentifier ?: @"";
                NSString *originalTransactionId = transaction.originalTransaction.transactionIdentifier ?: transactionId;
                NSString *productId = transaction.payment.productIdentifier ?: @"";

                qInfo().noquote() << "[IAP][StoreKit] Transaction restored"
                                  << QString::fromUtf8(transactionId.UTF8String)
                                  << "original="
                                  << QString::fromUtf8((originalTransactionId ?: @"").UTF8String)
                                  << "product="
                                  << QString::fromUtf8((productId ?: @"").UTF8String);

                NSDictionary *info = @{
                    @"transactionId": transactionId,
                    @"originalTransactionId": originalTransactionId ?: @"",
                    @"productId": productId ?: @""
                };
                if (!self.restoredTransactions) {
                    self.restoredTransactions = [NSMutableArray array];
                }
                [self.restoredTransactions addObject:info];
            }
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            break;
        }
        case SKPaymentTransactionStatePurchasing:
        case SKPaymentTransactionStateDeferred: 
            break;
        }
    }
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue
{
    if (self.restoreCompletion) {
        NSArray<NSDictionary *> *transactions = [self.restoredTransactions copy];
        self.restoreCompletion(YES, transactions, nil);
        self.restoreCompletion = nil;
        self.restoredTransactions = nil;
    }
}

- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error
{
    if (self.restoreCompletion) {
        self.restoreCompletion(NO, nil, error);
        self.restoreCompletion = nil;
        self.restoredTransactions = nil;
    }
}

@end
