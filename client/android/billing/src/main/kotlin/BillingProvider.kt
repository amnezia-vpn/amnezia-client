import android.app.Activity
import android.content.Context
import com.android.billingclient.api.AcknowledgePurchaseParams
import com.android.billingclient.api.BillingClient
import com.android.billingclient.api.BillingClient.BillingResponseCode
import com.android.billingclient.api.BillingClient.ProductType
import com.android.billingclient.api.BillingClientStateListener
import com.android.billingclient.api.BillingFlowParams
import com.android.billingclient.api.BillingFlowParams.SubscriptionUpdateParams.ReplacementMode
import com.android.billingclient.api.BillingResult
import com.android.billingclient.api.GetBillingConfigParams
import com.android.billingclient.api.PendingPurchasesParams
import com.android.billingclient.api.ProductDetails
import com.android.billingclient.api.Purchase
import com.android.billingclient.api.PurchasesUpdatedListener
import com.android.billingclient.api.QueryProductDetailsParams
import com.android.billingclient.api.QueryProductDetailsParams.Product
import com.android.billingclient.api.QueryPurchasesParams
import com.android.billingclient.api.acknowledgePurchase
import com.android.billingclient.api.queryProductDetails
import com.android.billingclient.api.queryPurchasesAsync
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.firstOrNull
import kotlinx.coroutines.withContext
import org.amnezia.vpn.util.ErrorCode
import org.amnezia.vpn.util.Log
import org.json.JSONArray
import org.json.JSONObject

private const val TAG = "BillingProvider"
private const val PRODUCT_ID = "premium"

class BillingProvider(context: Context) : AutoCloseable {

    private var billingClient: BillingClient
    private var subscriptionPurchases = MutableStateFlow<Pair<BillingResult, List<Purchase>?>?>(null)

    private val purchasesUpdatedListeners = PurchasesUpdatedListener { billingResult, purchases ->
        Log.v(TAG, "Purchases updated: $billingResult")
        subscriptionPurchases.value = billingResult to purchases
    }

    init {
        billingClient = BillingClient.newBuilder(context)
            .setListener(purchasesUpdatedListeners)
            .enablePendingPurchases(PendingPurchasesParams.newBuilder().enableOneTimeProducts().build())
            .build()
    }

    private suspend fun connect() {
        if (billingClient.isReady) return

        Log.v(TAG, "Billing client connection")
        val connection = CompletableDeferred<Unit>()
        withContext(Dispatchers.IO) {
            billingClient.startConnection(object : BillingClientStateListener {
                override fun onBillingSetupFinished(billingResult: BillingResult) {
                    Log.v(TAG, "Billing setup finished: $billingResult")
                    if (billingResult.isOk) {
                        connection.complete(Unit)
                    } else {
                        Log.e(TAG, "Billing setup failed: $billingResult")
                        connection.completeExceptionally(BillingException(billingResult))
                    }
                }

                override fun onBillingServiceDisconnected() {
                    Log.w(TAG, "Billing service disconnected")
                }
            })
        }
        connection.await()
    }

    private suspend fun handleBillingApiCall(block: suspend () -> JSONObject): JSONObject {
        val numberAttempts = 3
        var attemptCount = 0
        while (true) {
            try {
                return block()
            } catch (e: BillingException) {
                if (e.isCanceled) {
                    Log.w(TAG, "Billing canceled")
                    return JSONObject().put("responseCode", ErrorCode.BillingCanceled)
                } else if (e.isRetryable && attemptCount < numberAttempts) {
                    Log.d(TAG, "Retryable error: $e")
                    ++attemptCount
                    delay(1000)
                } else {
                    Log.e(TAG, "Billing error: $e")
                    return JSONObject().put("responseCode", e.errorCode)
                }
            } catch (_: CancellationException) {
                Log.w(TAG, "Billing coroutine canceled")
                return JSONObject().put("responseCode", ErrorCode.BillingCanceled)
            }
        }
    }

    suspend fun getSubscriptionPlans(): JSONObject {
        Log.v(TAG, "Get subscription plans")

        val productDetailsList = getProductDetails()
        val resultJson = JSONObject().put("responseCode", ErrorCode.NoError)
        val productArray = JSONArray().also { resultJson.put("products", it) }
        productDetailsList?.forEach { productDetails ->
            val product = JSONObject().also { productArray.put(it) }
                .put("productId", productDetails.productId)
                .put("name", productDetails.name)
            val offers = JSONArray().also { product.put("offers", it) }
            productDetails.subscriptionOfferDetails?.forEach { offerDetails ->
                val offer = JSONObject().also { offers.put(it) }
                    .put("basePlanId", offerDetails.basePlanId)
                    .put("offerId", offerDetails.offerId)
                    .put("offerToken", offerDetails.offerToken)
                val pricingPhases = JSONArray().also { offer.put("pricingPhases", it) }
                offerDetails.pricingPhases.pricingPhaseList.forEach { phase ->
                    JSONObject().also { pricingPhases.put(it) }
                        .put("billingCycleCount", phase.billingCycleCount)
                        .put("billingPeriod", phase.billingPeriod)
                        .put("formatedPrice", phase.formattedPrice)
                        .put("recurrenceMode", phase.recurrenceMode)
                }
            }
        }
        return resultJson
    }

    private suspend fun getProductDetails(): List<ProductDetails>? {
        Log.v(TAG, "Get product details")

        val productDetailsParams = Product.newBuilder()
            .setProductId(PRODUCT_ID)
            .setProductType(ProductType.SUBS)
            .build()

        val queryProductDetailsParams = QueryProductDetailsParams.newBuilder()
            .setProductList(listOf(productDetailsParams))
            .build()

        val result = withContext(Dispatchers.IO) {
            billingClient.queryProductDetails(queryProductDetailsParams)
        }

        Log.v(TAG, "Query product details result: ${result.billingResult}")

        if (!result.billingResult.isOk) {
            Log.e(TAG, "Failed to get product details: ${result.billingResult}")
            throw BillingException(result.billingResult)
        }

        return result.productDetailsList
    }

    suspend fun getCustomerCountryCode(): JSONObject {
        Log.v(TAG, "Get customer country code")

        val deferred = CompletableDeferred<String>()
        withContext(Dispatchers.IO) {
            billingClient.getBillingConfigAsync(GetBillingConfigParams.newBuilder().build(),
                { billingResult, billingConfig ->
                    Log.v(TAG, "Billing config: $billingResult, ${billingConfig?.countryCode}")
                    if (billingResult.isOk) {
                        deferred.complete(billingConfig?.countryCode ?: "")
                    } else {
                        deferred.completeExceptionally(BillingException(billingResult))
                    }
                })
        }
        val countryCode = deferred.await()

        return JSONObject()
            .put("responseCode", ErrorCode.NoError)
            .put("countryCode", countryCode)
    }

    suspend fun purchaseSubscription(
        activity: Activity,
        offerToken: String,
        oldPurchaseToken: String? = null
    ): JSONObject {
        Log.v(TAG, "Purchase subscription")
        Log.v(TAG, "Offer token: $offerToken")
        oldPurchaseToken?.let { Log.v(TAG, "Old purchase token: $it") }

        if (offerToken.isBlank()) throw BillingException("offerToken can not be empty")

        val productDetails = getProductDetails()?.let {
            it.filter { it.productId == PRODUCT_ID }
        }?.firstOrNull() ?: throw BillingException("Product details not found")

        Log.v(TAG, "Filtered product details:\n$productDetails")

        val productDetail = BillingFlowParams.ProductDetailsParams.newBuilder()
            .setProductDetails(productDetails)
            .setOfferToken(offerToken)
            .build()

        val subscriptionUpdateParams = oldPurchaseToken?.let {
            BillingFlowParams.SubscriptionUpdateParams.newBuilder()
                .setOldPurchaseToken(oldPurchaseToken)
                .setSubscriptionReplacementMode(ReplacementMode.WITHOUT_PRORATION)
                .build()
        }

        val billingResult = billingClient.launchBillingFlow(activity, BillingFlowParams.newBuilder()
            .setProductDetailsParamsList(listOf(productDetail))
            .apply { subscriptionUpdateParams?.let { setSubscriptionUpdateParams(it) } }
            .build())

        Log.v(TAG, "Start billing flow result: $billingResult")

        if (billingResult.responseCode == BillingResponseCode.ITEM_ALREADY_OWNED) {
            Log.w(TAG, "Attempting to purchase already owned product")
            val purchases = queryPurchases()
            if (purchases.any { PRODUCT_ID in it.products }) throw BillingException(billingResult)
            else throw BillingException(billingResult, retryable = true)
        } else if (billingResult.responseCode == BillingResponseCode.ITEM_NOT_OWNED) {
            Log.w(TAG, "Attempting to replace not owned product")
            val purchases = queryPurchases()
            if (purchases.all { PRODUCT_ID !in it.products }) throw BillingException(billingResult)
            else throw BillingException(billingResult, retryable = true)
        } else if (!billingResult.isOk) throw BillingException(billingResult)

        subscriptionPurchases.firstOrNull { it != null }?.let { (billingResult, purchases) ->
            if (!billingResult.isOk) throw BillingException(billingResult)
            return JSONObject()
                .put("responseCode", ErrorCode.NoError)
                .put("purchases", processPurchases(purchases))
        } ?: throw BillingException("Purchase failed")
    }

    private fun processPurchases(purchases: List<Purchase>?): JSONArray {
        val purchaseArray = JSONArray()
        purchases?.forEach { purchase ->
            /* val purchaseJson = */ JSONObject().also { purchaseArray.put(it) }
                .put("purchaseToken", purchase.purchaseToken)
                .put("purchaseTime", purchase.purchaseTime)
                .put("purchaseState", purchase.purchaseState)
                .put("isAcknowledged", purchase.isAcknowledged)
                .put("isAutoRenewing", purchase.isAutoRenewing)
                .put("orderId", purchase.orderId)
                // .put("productIds", JSONArray(purchase.products))

            /* purchase.pendingPurchaseUpdate?.let { purchaseUpdate ->
                JSONObject()
                    .put("purchaseToken", purchaseUpdate.purchaseToken)
                    // .put("productIds", JSONArray(purchaseUpdate.products))
            }.also { purchaseJson.put("pendingPurchaseUpdate", it) } */
        }
        return purchaseArray
    }

    suspend fun acknowledge(purchaseToken: String): JSONObject {
        Log.v(TAG, "Acknowledge purchase: $purchaseToken")

        val result = withContext(Dispatchers.IO) {
            billingClient.acknowledgePurchase(
                AcknowledgePurchaseParams.newBuilder()
                    .setPurchaseToken(purchaseToken)
                    .build()
            )
        }

        Log.v(TAG, "Acknowledge purchase result: $result")

        if (result.responseCode == BillingResponseCode.ITEM_NOT_OWNED) {
            Log.w(TAG, "Attempting to acknowledge not owned product")
            val purchases = queryPurchases()
            if (purchases.all { PRODUCT_ID !in it.products }) throw BillingException(result)
            else throw BillingException(result, retryable = true)
        } else if (!result.isOk && result.responseCode != BillingResponseCode.ITEM_ALREADY_OWNED) {
            throw BillingException(result)
        }

        return JSONObject().put("responseCode", ErrorCode.NoError)
    }

    suspend fun getPurchases(): JSONObject {
        Log.v(TAG, "Get purchases")
        val purchases = queryPurchases()
        return JSONObject()
            .put("responseCode", ErrorCode.NoError)
            .put("purchases", processPurchases(purchases))
    }

    private suspend fun queryPurchases(): List<Purchase> {
        Log.v(TAG, "Query purchases")
        val result = withContext(Dispatchers.IO) {
            billingClient.queryPurchasesAsync(
                QueryPurchasesParams.newBuilder().setProductType(ProductType.SUBS).build()
            )
        }
        Log.v(TAG, "Query purchases result: ${result.billingResult}")
        if (!result.billingResult.isOk) throw BillingException(result.billingResult)
        return result.purchasesList
    }

    override fun close() {
        Log.v(TAG, "Close billing client connection")
        billingClient.endConnection()
    }

    companion object {
        suspend fun withBillingProvider(context: Context, block: suspend BillingProvider.() -> JSONObject): String =
            BillingProvider(context).use { bp ->
                bp.handleBillingApiCall {
                    bp.connect()
                    bp.block()
                }.toString()
            }
    }
}

internal val BillingResult.isOk: Boolean
    get() = responseCode == BillingResponseCode.OK
