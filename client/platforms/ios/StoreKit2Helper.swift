import Foundation
import StoreKit

@available(iOS 15.0, macOS 12.0, *)
@objcMembers
public class StoreKit2Helper: NSObject {

    public static let shared = StoreKit2Helper()
    private static let errorDomain = "StoreKit2Helper"

    // Error codes surfaced to the native bridge; keep in sync with ios_controller.mm
    public static let errorCodeCancelled = 1
    public static let errorCodePending = 2

    private var updatesTask: Task<Void, Never>?

    private struct EntitlementInfo {
        let transactionId: UInt64
        let originalTransactionId: UInt64
        let productId: String
        let purchaseDate: Date
        let environment: String

        var dictionary: NSDictionary {
            [
                "transactionId": String(transactionId),
                "originalTransactionId": String(originalTransactionId),
                "productId": productId,
                "environment": environment
            ]
        }
    }

    private func environmentString(for transaction: Transaction) -> String {
        if #available(iOS 16.0, macOS 13.0, *) {
            switch transaction.environment {
            case .production: return "production"
            case .sandbox: return "sandbox"
            case .xcode: return "xcode"
            default: return transaction.environment.rawValue.lowercased()
            }
        }
        // Fallback for OS versions without Transaction.environment
        return Bundle.main.appStoreReceiptURL?.lastPathComponent == "sandboxReceipt" ? "sandbox" : "production"
    }

    private func collectCurrentEntitlements() async -> [NSDictionary] {
        var entitlements: [EntitlementInfo] = []
        for await result in Transaction.currentEntitlements {
            switch result {
            case .verified(let transaction):
                entitlements.append(EntitlementInfo(transactionId: transaction.id,
                                                    originalTransactionId: transaction.originalID,
                                                    productId: transaction.productID,
                                                    purchaseDate: transaction.purchaseDate,
                                                    environment: environmentString(for: transaction)))
            case .unverified(_, let error):
                print("[IAP][StoreKit2] Unverified transaction skipped: \(error.localizedDescription)")
            }
        }
        return entitlements.sorted { lhs, rhs in
            if lhs.purchaseDate != rhs.purchaseDate {
                return lhs.purchaseDate > rhs.purchaseDate
            }
            return lhs.transactionId > rhs.transactionId
        }.map { $0.dictionary }
    }

    public func fetchCurrentEntitlements(completion: @escaping (Bool, [NSDictionary]?, NSError?) -> Void) {
        Task { @MainActor in
            do {
                try await AppStore.sync()
                completion(true, await collectCurrentEntitlements(), nil)
            } catch {
                completion(false, nil, error as NSError)
            }
        }
    }

    public func fetchLocalEntitlements(completion: @escaping (Bool, [NSDictionary]?, NSError?) -> Void) {
        Task { @MainActor in
            completion(true, await collectCurrentEntitlements(), nil)
        }
    }

    // The transaction is intentionally NOT finished here: it must stay in the unfinished
    // queue until the gateway validates the purchase and the config is delivered.
    // Call finishTransaction(transactionId:) after successful validation.
    public func purchaseProduct(productIdentifier: String, completion: @escaping (Bool, String?, String?, String?, String?, NSError?) -> Void) {
        Task {
            do {
                let products = try await Product.products(for: [productIdentifier])
                guard let product = products.first else {
                    completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                     environment: nil, error: makeError(code: 0, description: "Product not found"))
                    return
                }
                let result = try await product.purchase()
                switch result {
                case .success(let verification):
                    switch verification {
                    case .verified(let transaction):
                        completePurchase(completion: completion, success: true, transactionId: String(transaction.id),
                                         productId: transaction.productID, originalTransactionId: String(transaction.originalID),
                                         environment: environmentString(for: transaction), error: nil)
                    case .unverified(_, let error):
                        completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                         environment: nil, error: error as NSError)
                    }
                case .userCancelled:
                    completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                     environment: nil, error: makeError(code: Self.errorCodeCancelled, description: "Purchase cancelled"))
                case .pending:
                    completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                     environment: nil, error: makeError(code: Self.errorCodePending, description: "Purchase pending"))
                @unknown default:
                    completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                     environment: nil, error: makeError(code: 3, description: "Unknown purchase result"))
                }
            } catch StoreKitError.userCancelled {
                completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                 environment: nil, error: makeError(code: Self.errorCodeCancelled, description: "Purchase cancelled"))
            } catch {
                completePurchase(completion: completion, success: false, transactionId: nil, productId: nil, originalTransactionId: nil,
                                 environment: nil, error: error as NSError)
            }
        }
    }

    public func finishTransaction(transactionId: String, completion: @escaping (Bool) -> Void) {
        Task {
            for await result in Transaction.unfinished {
                guard case .verified(let transaction) = result, String(transaction.id) == transactionId else {
                    continue
                }
                await transaction.finish()
                print("[IAP][StoreKit2] Finished transaction \(transactionId)")
                DispatchQueue.main.async { completion(true) }
                return
            }
            print("[IAP][StoreKit2] No unfinished transaction with id \(transactionId) to finish")
            DispatchQueue.main.async { completion(false) }
        }
    }

    @MainActor private var deliveredTransactionUpdateIds = Set<UInt64>()

    @MainActor
    private func verifiedTransaction(from result: VerificationResult<Transaction>) -> Transaction? {
        switch result {
        case .verified(let transaction):
            return transaction
        case .unverified(_, let error):
            print("[IAP][StoreKit2] Unverified transaction update skipped: \(error.localizedDescription)")
            return nil
        }
    }

    @MainActor
    private func finishStaleTransactionIfNeeded(_ transaction: Transaction) async -> Bool {
        let isRevoked = transaction.revocationDate != nil
        let isExpired = transaction.expirationDate.map { $0 <= Date() } ?? false
        guard isRevoked || isExpired else { return false }
        await transaction.finish()
        print("[IAP][StoreKit2] Finished \(isRevoked ? "revoked" : "expired") transaction id=\(transaction.id) product=\(transaction.productID)")
        return true
    }

    @MainActor
    private func deliverTransactionUpdate(_ transaction: Transaction, handler: @escaping (NSDictionary) -> Void) {
        guard deliveredTransactionUpdateIds.insert(transaction.id).inserted else { return }
        let info = EntitlementInfo(transactionId: transaction.id,
                                   originalTransactionId: transaction.originalID,
                                   productId: transaction.productID,
                                   purchaseDate: transaction.purchaseDate,
                                   environment: environmentString(for: transaction))
        print("[IAP][StoreKit2] Transaction update: id=\(transaction.id) product=\(transaction.productID)")
        handler(info.dictionary)
    }

    public func startTransactionUpdatesListener(handler: @escaping (NSDictionary) -> Void) {
        guard updatesTask == nil else { return }

        updatesTask = Task { @MainActor in
            for await result in Transaction.updates {
                guard let transaction = self.verifiedTransaction(from: result) else { continue }
                if await self.finishStaleTransactionIfNeeded(transaction) { continue }
                self.deliverTransactionUpdate(transaction, handler: handler)
            }
        }

        Task { @MainActor in
            var newestPerSubscription: [UInt64: Transaction] = [:]
            for await result in Transaction.unfinished {
                guard let transaction = self.verifiedTransaction(from: result) else { continue }
                if await self.finishStaleTransactionIfNeeded(transaction) { continue }

                guard let current = newestPerSubscription[transaction.originalID] else {
                    newestPerSubscription[transaction.originalID] = transaction
                    continue
                }
                let duplicate: Transaction
                if (current.purchaseDate, current.id) < (transaction.purchaseDate, transaction.id) {
                    newestPerSubscription[transaction.originalID] = transaction
                    duplicate = current
                } else {
                    duplicate = transaction
                }
                deliveredTransactionUpdateIds.insert(duplicate.id)
                print("[IAP][StoreKit2] Suppressing duplicate unfinished transaction id=\(duplicate.id) (originalTransactionId=\(duplicate.originalID))")
            }
            for transaction in newestPerSubscription.values {
                self.deliverTransactionUpdate(transaction, handler: handler)
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

    private func completePurchase(completion: @escaping (Bool, String?, String?, String?, String?, NSError?) -> Void,
                                  success: Bool,
                                  transactionId: String?,
                                  productId: String?,
                                  originalTransactionId: String?,
                                  environment: String?,
                                  error: NSError?) {
        DispatchQueue.main.async {
            completion(success, transactionId, productId, originalTransactionId, environment, error)
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
