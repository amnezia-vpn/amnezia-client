/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "platforms/ios/iosiaphandler.h"
#include "constants.h"
#include "iosutils.h"
#include "leakdetector.h"
#include "logger.h"
#include "mozillavpn.h"
#include "networkrequest.h"
#include "settingsholder.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QScopeGuard>

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

namespace {
Logger logger(LOG_IAP, "IOSIAPHandler");
}  // namespace

@interface IOSIAPHandlerDelegate
    : NSObject <SKRequestDelegate, SKProductsRequestDelegate, SKPaymentTransactionObserver> {
  IOSIAPHandler* m_handler;
}
@end

@implementation IOSIAPHandlerDelegate

- (id)initWithObject:(IOSIAPHandler*)handler {
  self = [super init];
  if (self) {
    m_handler = handler;
  }
  return self;
}

- (void)productsRequest:(nonnull SKProductsRequest*)request
     didReceiveResponse:(nonnull SKProductsResponse*)response {
  logger.debug() << "Registration completed";

  if (response.invalidProductIdentifiers) {
    NSArray<NSString*>* products = response.invalidProductIdentifiers;
    logger.error() << "Registration failure" << [products count];

    for (unsigned long i = 0, count = [products count]; i < count; ++i) {
      NSString* identifier = [products objectAtIndex:i];
      QMetaObject::invokeMethod(m_handler, "unknownProductRegistered", Qt::QueuedConnection,
                                Q_ARG(QString, QString::fromNSString(identifier)));
    }
  }

  NSArray<SKProduct*>* products = response.products;
  if (products) {
    logger.debug() << "Products registered" << [products count];

    for (unsigned long i = 0, count = [products count]; i < count; ++i) {
      SKProduct* product = [[products objectAtIndex:i] retain];
      QMetaObject::invokeMethod(m_handler, "productRegistered", Qt::QueuedConnection,
                                Q_ARG(void*, product));
    }
  }

  QMetaObject::invokeMethod(m_handler, "productsRegistrationCompleted", Qt::QueuedConnection);

  [request release];
}

- (void)paymentQueue:(nonnull SKPaymentQueue*)queue
    updatedTransactions:(nonnull NSArray<SKPaymentTransaction*>*)transactions {
  logger.debug() << "payment queue:" << [transactions count];

  QStringList completedTransactionIds;
  bool failedTransactions = false;
  bool canceledTransactions = false;
  bool completedTransactions = false;

  for (SKPaymentTransaction* transaction in transactions) {
    switch (transaction.transactionState) {
      case SKPaymentTransactionStateFailed:
        logger.error() << "transaction failed";

        if (transaction.error.code == SKErrorPaymentCancelled) {
          canceledTransactions = true;
        } else {
          failedTransactions = true;
        }
        break;

      case SKPaymentTransactionStateRestored:
        [[fallthrough]];
      case SKPaymentTransactionStatePurchased: {
        QString identifier = QString::fromNSString(transaction.transactionIdentifier);
        QDateTime date = QDateTime::fromNSDate(transaction.transactionDate);
        logger.debug() << "transaction purchased - identifier: " << identifier
                       << "- date:" << date.toString();

        if (transaction.transactionState == SKPaymentTransactionStateRestored) {
          SKPaymentTransaction* originalTransaction = transaction.originalTransaction;
          if (originalTransaction) {
            QString originalIdentifier =
                QString::fromNSString(originalTransaction.transactionIdentifier);
            QDateTime originalDate = QDateTime::fromNSDate(originalTransaction.transactionDate);
            logger.debug() << "original transaction identifier: " << originalIdentifier
                           << "- date:" << originalDate.toString();
          }
        }

        completedTransactions = true;

        SettingsHolder* settingsHolder = SettingsHolder::instance();
        if (settingsHolder->hasSubscriptionTransaction(identifier)) {
          logger.warning() << "This transaction has already been processed. Let's ignore it.";
        } else {
          completedTransactionIds.append(identifier);
        }

        break;
      }
      case SKPaymentTransactionStatePurchasing:
        logger.debug() << "transaction purchasing";
        break;
      case SKPaymentTransactionStateDeferred:
        logger.debug() << "transaction deferred";
        break;
      default:
        logger.warning() << "transaction unknown state";
        break;
    }
  }

  if (!completedTransactions && !canceledTransactions && !failedTransactions) {
    // Nothing completed, nothing restored, nothing failed. Just purchasing transactions.
    return;
  }

  if (canceledTransactions) {
    logger.debug() << "Subscription canceled";
    QMetaObject::invokeMethod(m_handler, "stopSubscription", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_handler, "subscriptionCanceled", Qt::QueuedConnection);
  } else if (failedTransactions) {
    logger.error() << "Subscription failed";
    QMetaObject::invokeMethod(m_handler, "stopSubscription", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_handler, "subscriptionCanceled", Qt::QueuedConnection);
  } else if (completedTransactionIds.isEmpty()) {
    Q_ASSERT(completedTransactions);
    logger.debug() << "Subscription completed - but all the transactions are known";
    QMetaObject::invokeMethod(m_handler, "stopSubscription", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_handler, "subscriptionCanceled", Qt::QueuedConnection);
  } else if (AmneziaVPN::instance()->userAuthenticated()) {
    Q_ASSERT(completedTransactions);
    logger.debug() << "Subscription completed. Let's start the validation";
    QMetaObject::invokeMethod(m_handler, "processCompletedTransactions", Qt::QueuedConnection,
                              Q_ARG(QStringList, completedTransactionIds));
  } else {
    Q_ASSERT(completedTransactions);
    logger.debug() << "Subscription completed - but the user is not authenticated yet";
    QMetaObject::invokeMethod(m_handler, "stopSubscription", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_handler, "subscriptionCanceled", Qt::QueuedConnection);
  }

  for (SKPaymentTransaction* transaction in transactions) {
    switch (transaction.transactionState) {
      case SKPaymentTransactionStateFailed:
        [[fallthrough]];
      case SKPaymentTransactionStateRestored:
        [[fallthrough]];
      case SKPaymentTransactionStatePurchased:
        [queue finishTransaction:transaction];
        break;
      default:
        break;
    }
  }
}

- (void)requestDidFinish:(SKRequest*)request {
  logger.debug() << "Receipt refreshed correctly";
  QMetaObject::invokeMethod(m_handler, "stopSubscription", Qt::QueuedConnection);
  QMetaObject::invokeMethod(m_handler, "processCompletedTransactions", Qt::QueuedConnection,
                            Q_ARG(QStringList, QStringList()));
}

- (void)request:(SKRequest*)request didFailWithError:(NSError*)error {
  logger.error() << "Failed to refresh the receipt"
                 << QString::fromNSString(error.localizedDescription);
  QMetaObject::invokeMethod(m_handler, "stopSubscription", Qt::QueuedConnection);
  QMetaObject::invokeMethod(m_handler, "subscriptionFailed", Qt::QueuedConnection);
}

@end

IOSIAPHandler::IOSIAPHandler(QObject* parent) : IAPHandler(parent) {
  MVPN_COUNT_CTOR(IOSIAPHandler);

  m_delegate = [[IOSIAPHandlerDelegate alloc] initWithObject:this];
  [[SKPaymentQueue defaultQueue]
      addTransactionObserver:static_cast<IOSIAPHandlerDelegate*>(m_delegate)];
}

IOSIAPHandler::~IOSIAPHandler() {
  MVPN_COUNT_DTOR(IOSIAPHandler);

  IOSIAPHandlerDelegate* delegate = static_cast<IOSIAPHandlerDelegate*>(m_delegate);
  [[SKPaymentQueue defaultQueue] removeTransactionObserver:delegate];

  [delegate dealloc];
  m_delegate = nullptr;
}

void IOSIAPHandler::nativeRegisterProducts() {
  NSSet<NSString*>* productIdentifiers = [NSSet<NSString*> set];
  for (const Product& product : m_products) {
    productIdentifiers = [productIdentifiers setByAddingObject:product.m_name.toNSString()];
  }

  logger.debug() << "We are about to register" << [productIdentifiers count] << "products";

  SKProductsRequest* productsRequest =
      [[SKProductsRequest alloc] initWithProductIdentifiers:productIdentifiers];

  IOSIAPHandlerDelegate* delegate = static_cast<IOSIAPHandlerDelegate*>(m_delegate);
  productsRequest.delegate = delegate;
  [productsRequest start];
}

void IOSIAPHandler::nativeStartSubscription(Product* product) {
  Q_ASSERT(product->m_extra);
  SKProduct* skProduct = static_cast<SKProduct*>(product->m_extra);
  SKPayment* payment = [SKPayment paymentWithProduct:skProduct];
  [[SKPaymentQueue defaultQueue] addPayment:payment];
}

void IOSIAPHandler::productRegistered(void* a_product) {
  SKProduct* product = static_cast<SKProduct*>(a_product);

  Q_ASSERT(m_productsRegistrationState == eRegistering);

  logger.debug() << "Product registered";

  NSString* nsProductIdentifier = [product productIdentifier];
  QString productIdentifier = QString::fromNSString(nsProductIdentifier);

  Product* productData = findProduct(productIdentifier);
  Q_ASSERT(productData);

  logger.debug() << "Id:" << productIdentifier;
  logger.debug() << "Title:" << QString::fromNSString([product localizedTitle]);
  logger.debug() << "Description:" << QString::fromNSString([product localizedDescription]);

  QString priceValue;
  {
    NSNumberFormatter* numberFormatter = [[NSNumberFormatter alloc] init];
    [numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
    [numberFormatter setNumberStyle:(NSNumberFormatterStyle)NSNumberFormatterCurrencyStyle];
    [numberFormatter setLocale:product.priceLocale];

    NSString* price = [numberFormatter stringFromNumber:product.price];
    priceValue = QString::fromNSString(price);
    [numberFormatter release];
  }

  logger.debug() << "Price:" << priceValue;

  QString monthlyPriceValue;
  NSDecimalNumber* monthlyPriceNS = nullptr;
  {
    NSNumberFormatter* numberFormatter = [[NSNumberFormatter alloc] init];
    [numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
    [numberFormatter setNumberStyle:(NSNumberFormatterStyle)NSNumberFormatterCurrencyStyle];
    [numberFormatter setLocale:product.priceLocale];

    int32_t mounthCount = productTypeToMonthCount(productData->m_type);
    Q_ASSERT(mounthCount >= 1);

    if (mounthCount == 1) {
      monthlyPriceNS = product.price;
    } else {
      NSDecimalNumber* divider = [[NSDecimalNumber alloc] initWithDouble:(double)mounthCount];
      monthlyPriceNS = [product.price decimalNumberByDividingBy:divider];
      [divider release];
    }

    NSString* price = [numberFormatter stringFromNumber:monthlyPriceNS];
    monthlyPriceValue = QString::fromNSString(price);

    [numberFormatter release];
  }

  logger.debug() << "Monthly Price:" << monthlyPriceValue;

  productData->m_price = priceValue;
  productData->m_monthlyPrice = monthlyPriceValue;
  productData->m_nonLocalizedMonthlyPrice = [monthlyPriceNS doubleValue];
  productData->m_extra = product;
}

void IOSIAPHandler::processCompletedTransactions(const QStringList& ids) {
  logger.debug() << "process completed transactions";

  if (m_subscriptionState != eActive) {
    logger.warning() << "Random transaction to be completed. Let's ignore it";
    return;
  }

  QString receipt = IOSUtils::IAPReceipt();
  if (receipt.isEmpty()) {
    logger.warning() << "Empty receipt found";
    emit subscriptionFailed();
    return;
  }

  NetworkRequest* request = NetworkRequest::createForIOSPurchase(this, receipt);

  connect(request, &NetworkRequest::requestFailed,
          [this](QNetworkReply::NetworkError error, const QByteArray& data) {
            logger.error() << "Purchase request failed" << error;

            if (m_subscriptionState != eActive) {
              logger.warning() << "We have been canceled in the meantime";
              return;
            }

            stopSubscription();

            QJsonDocument json = QJsonDocument::fromJson(data);
            if (!json.isObject()) {
              AmneziaVPN::instance()->errorHandle(ErrorHandler::toErrorType(error));
              emit subscriptionFailed();
              return;
            }

            QJsonObject obj = json.object();
            QJsonValue errorValue = obj.value("errno");
            if (!errorValue.isDouble()) {
              AmneziaVPN::instance()->errorHandle(ErrorHandler::toErrorType(error));
              emit subscriptionFailed();
              return;
            }

            int errorNumber = errorValue.toInt();
            if (errorNumber != 145) {
              AmneziaVPN::instance()->errorHandle(ErrorHandler::toErrorType(error));
              emit subscriptionFailed();
              return;
            }

            emit alreadySubscribed();
          });

  connect(request, &NetworkRequest::requestCompleted, [this, ids](const QByteArray&) {
    logger.debug() << "Purchase request completed";
    SettingsHolder::instance()->addSubscriptionTransactions(ids);

    if (m_subscriptionState != eActive) {
      logger.warning() << "We have been canceled in the meantime";
      return;
    }

    stopSubscription();
    emit subscriptionCompleted();
  });
}
