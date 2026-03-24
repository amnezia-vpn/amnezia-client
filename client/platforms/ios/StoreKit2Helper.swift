import Foundation
import StoreKit

@available(iOS 15.0, macOS 12.0, *)
@objcMembers
public class StoreKit2Helper: NSObject {

    public static let shared = StoreKit2Helper()

    public func fetchCurrentEntitlements(completion: @escaping (Bool, [NSDictionary]?, NSError?) -> Void) {
        Task {
            var entitlements: [NSDictionary] = []
            do {
                for await result in Transaction.currentEntitlements {
                    switch result {
                    case .verified(let transaction):
                        let info: NSDictionary = [
                            "transactionId": String(transaction.id),
                            "originalTransactionId": String(transaction.originalID),
                            "productId": transaction.productID
                        ]
                        entitlements.append(info)
                    case .unverified(_, let error):
                        print("[IAP][StoreKit2] Unverified transaction skipped: \(error.localizedDescription)")
                    }
                }
                DispatchQueue.main.async { completion(true, entitlements, nil) }
            }
        }
    }

    public func purchaseProduct(productIdentifier: String, completion: @escaping (Bool, String?, String?, String?, NSError?) -> Void) {
        Task {
            do {
                let products = try await Product.products(for: [productIdentifier])
                guard let product = products.first else {
                    let error = NSError(domain: "StoreKit2Helper", code: 0, userInfo: [NSLocalizedDescriptionKey: "Product not found"])
                    DispatchQueue.main.async { completion(false, nil, nil, nil, error) }
                    return
                }
                let result = try await product.purchase()
                switch result {
                case .success(let verification):
                    switch verification {
                    case .verified(let transaction):
                        await transaction.finish()
                        let txId = String(transaction.id)
                        let origTxId = String(transaction.originalID)
                        let pId = transaction.productID
                        DispatchQueue.main.async { completion(true, txId, pId, origTxId, nil) }
                    case .unverified(_, let error):
                        DispatchQueue.main.async { completion(false, nil, nil, nil, error as NSError) }
                    }
                case .userCancelled:
                    let error = NSError(domain: "StoreKit2Helper", code: 1, userInfo: [NSLocalizedDescriptionKey: "Purchase cancelled"])
                    DispatchQueue.main.async { completion(false, nil, nil, nil, error) }
                case .pending:
                    let error = NSError(domain: "StoreKit2Helper", code: 2, userInfo: [NSLocalizedDescriptionKey: "Purchase pending"])
                    DispatchQueue.main.async { completion(false, nil, nil, nil, error) }
                @unknown default:
                    let error = NSError(domain: "StoreKit2Helper", code: 3, userInfo: [NSLocalizedDescriptionKey: "Unknown purchase result"])
                    DispatchQueue.main.async { completion(false, nil, nil, nil, error) }
                }
            } catch {
                DispatchQueue.main.async { completion(false, nil, nil, nil, error as NSError) }
            }
        }
    }

    private func storefrontCurrencyCode(for product: Product) -> String {
        product.priceFormatStyle.locale.currencyCode ?? ""
    }

    public func fetchProducts(identifiers: Set<String>, completion: @escaping ([NSDictionary], [String], NSError?) -> Void) {
        Task {
            do {
                let products = try await Product.products(for: identifiers)
                let productDicts = products.map { product -> NSDictionary in
                    let currencyCode = storefrontCurrencyCode(for: product)
                    return [
                        "productId": product.id,
                        "title": product.displayName,
                        "description": product.description,
                        "price": "\(product.price)",
                        "currencyCode": currencyCode
                    ]
                }
                let fetchedIds = Set(products.map { $0.id })
                let invalidIdentifiers = identifiers.filter { !fetchedIds.contains($0) }
                DispatchQueue.main.async { completion(productDicts, Array(invalidIdentifiers), nil) }
            } catch {
                DispatchQueue.main.async { completion([], Array(identifiers), error as NSError) }
            }
        }
    }
}
