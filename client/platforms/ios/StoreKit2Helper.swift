import Foundation
import StoreKit

@available(iOS 15.0, macOS 12.0, *)
@objcMembers
public class StoreKit2Helper: NSObject {

    public static let shared = StoreKit2Helper()
    private static let errorDomain = "StoreKit2Helper"

    private struct EntitlementInfo {
        let transactionId: UInt64
        let originalTransactionId: UInt64
        let productId: String
        let purchaseDate: Date

        var dictionary: NSDictionary {
            [
                "transactionId": String(transactionId),
                "originalTransactionId": String(originalTransactionId),
                "productId": productId
            ]
        }
    }

    public func fetchCurrentEntitlements(completion: @escaping (Bool, [NSDictionary]?, NSError?) -> Void) {
        Task { @MainActor in
            do {
                try await AppStore.sync()

                var entitlements: [EntitlementInfo] = []
                for await result in Transaction.currentEntitlements {
                    switch result {
                    case .verified(let transaction):
                        entitlements.append(EntitlementInfo(transactionId: transaction.id,
                                                            originalTransactionId: transaction.originalID,
                                                            productId: transaction.productID,
                                                            purchaseDate: transaction.purchaseDate))
                    case .unverified(_, let error):
                        print("[IAP][StoreKit2] Unverified transaction skipped: \(error.localizedDescription)")
                    }
                }
                let sortedEntitlements = entitlements.sorted { lhs, rhs in
                    if lhs.purchaseDate != rhs.purchaseDate {
                        return lhs.purchaseDate > rhs.purchaseDate
                    }
                    return lhs.transactionId > rhs.transactionId
                }.map { $0.dictionary }
                completion(true, sortedEntitlements, nil)
            } catch {
                completion(false, nil, error as NSError)
            }
        }
    }

    public func purchaseProduct(productIdentifier: String, completion: @escaping (Bool, String?, String?, String?, NSError?) -> Void) {
        Task {
            do {
                let products = try await Product.products(for: [productIdentifier])
                guard let product = products.first else {
                    completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                     error: makeError(code: 0, description: "Product not found"))
                    return
                }
                let result = try await product.purchase()
                switch result {
                case .success(let verification):
                    switch verification {
                    case .verified(let transaction):
                        await transaction.finish()
                        completePurchase(completion: completion, success: true, transactionId: String(transaction.id),
                                         productId: transaction.productID, originalTransactionId: String(transaction.originalID), error: nil)
                    case .unverified(_, let error):
                        completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                         error: error as NSError)
                    }
                case .userCancelled:
                    completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                     error: makeError(code: 1, description: "Purchase cancelled"))
                case .pending:
                    completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                     error: makeError(code: 2, description: "Purchase pending"))
                @unknown default:
                    completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                     error: makeError(code: 3, description: "Unknown purchase result"))
                }
            } catch {
                completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                 error: error as NSError)
            }
        }
    }

    private func storefrontCurrencyCode(for product: Product) -> String {
        product.priceFormatStyle.locale.currencyCode ?? ""
    }

    private func subscriptionBillingMonths(_ period: Product.SubscriptionPeriod) -> Double {
        let periodValue = Double(period.value)
        switch period.unit {
        case .day:
            return periodValue / 30.0
        case .week:
            return periodValue * 7.0 / 30.0
        case .month:
            return periodValue
        case .year:
            return periodValue * 12.0
        @unknown default:
            return periodValue
        }
    }

    public func fetchProducts(identifiers: Set<String>, completion: @escaping ([NSDictionary], [String], NSError?) -> Void) {
        Task {
            do {
                let products = try await Product.products(for: identifiers)
                var productDicts: [NSDictionary] = []
                for product in products {
                    productDicts.append(await productDictionary(for: product))
                }
                let fetchedIds = Set(products.map { $0.id })
                let invalidIdentifiers = identifiers.filter { !fetchedIds.contains($0) }
                DispatchQueue.main.async { completion(productDicts, Array(invalidIdentifiers), nil) }
            } catch {
                DispatchQueue.main.async { completion([], Array(identifiers), error as NSError) }
            }
        }
    }

    private func makeError(code: Int, description: String) -> NSError {
        NSError(domain: Self.errorDomain, code: code, userInfo: [NSLocalizedDescriptionKey: description])
    }

    private func completePurchase(completion: @escaping (Bool, String?, String?, String?, NSError?) -> Void,
                                  success: Bool,
                                  transactionId: String?,
                                  productId: String?,
                                  originalTransactionId: String?,
                                  error: NSError?) {
        DispatchQueue.main.async {
            completion(success, transactionId, productId, originalTransactionId, error)
        }
    }

    private func introOfferPaymentModeString(_ mode: Product.SubscriptionOffer.PaymentMode) -> String {
        switch mode {
        case .freeTrial:
            return "freeTrial"
        case .payAsYouGo:
            return "payAsYouGo"
        case .payUpFront:
            return "payUpFront"
        default:
            return "unknown"
        }
    }

    private func introOfferTrialDays(_ period: Product.SubscriptionPeriod) -> Int {
        switch period.unit {
        case .day:
            return period.value
        case .week:
            return period.value * 7
        case .month:
            return period.value * 30
        case .year:
            return period.value * 365
        @unknown default:
            return period.value
        }
    }

    private func productDictionary(for product: Product) async -> NSDictionary {
        let currencyCode = storefrontCurrencyCode(for: product)
        var productData: [String: Any] = [
            "productId": product.id,
            "title": product.displayName,
            "description": product.description,
            "price": "\(product.price)",
            "displayPrice": product.displayPrice,
            "currencyCode": currencyCode,
            "priceAmount": NSDecimalNumber(decimal: product.price).doubleValue
        ]
        if let subscription = product.subscription {
            let billingMonths = subscriptionBillingMonths(subscription.subscriptionPeriod)
            productData["subscriptionBillingMonths"] = billingMonths
            if let perMonthPrice = displayPricePerMonth(for: product, billingMonths: billingMonths, currencyCode: currencyCode) {
                productData["displayPricePerMonth"] = perMonthPrice
            }

            if let introOffer = subscription.introductoryOffer, await subscription.isEligibleForIntroOffer {
                if introOffer.paymentMode == .freeTrial {
                    productData["hasFreeTrial"] = true
                    productData["trialDays"] = introOfferTrialDays(introOffer.period)
                } else {
                    productData["introOfferDisplayPrice"] = introOffer.displayPrice
                    productData["introOfferPaymentMode"] = introOfferPaymentModeString(introOffer.paymentMode)
                }
            }
        }
        return productData as NSDictionary
    }

    private func displayPricePerMonth(for product: Product, billingMonths: Double, currencyCode: String) -> String? {
        if billingMonths <= 1e-6 {
            return nil
        }

        let perMonthPrice = product.price / Decimal(billingMonths)
        let formatter = NumberFormatter()
        formatter.numberStyle = .currency
        formatter.locale = product.priceFormatStyle.locale
        if !currencyCode.isEmpty {
            formatter.currencyCode = currencyCode
        }
        return formatter.string(from: NSDecimalNumber(decimal: perMonthPrice))
    }
}
