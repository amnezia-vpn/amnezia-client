import android.app.Activity
import android.content.Context
import com.android.billingclient.api.BillingClient
import com.android.billingclient.api.BillingClient.BillingResponseCode
import com.android.billingclient.api.BillingClient.ProductType
import com.android.billingclient.api.BillingClientStateListener
import com.android.billingclient.api.BillingFlowParams
import com.android.billingclient.api.BillingResult
import com.android.billingclient.api.GetBillingConfigParams
import com.android.billingclient.api.PendingPurchasesParams
import com.android.billingclient.api.Purchase
import com.android.billingclient.api.PurchasesUpdatedListener
import com.android.billingclient.api.QueryProductDetailsParams
import com.android.billingclient.api.QueryProductDetailsParams.Product
import com.android.billingclient.api.queryProductDetails
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.withContext
import org.amnezia.vpn.util.Log
import org.json.JSONArray
import org.json.JSONObject

private const val TAG = "BillingProvider"
private const val RESULT_OK = 1
private const val RESULT_CANCELED = 0
private const val RESULT_ERROR = -1

class BillingProvider(context: Context) : AutoCloseable {

    private var billingClient: BillingClient
    private var subscriptionPurchases = MutableStateFlow<Pair<BillingResult, List<Purchase>?>?>(null)

    private val purchasesUpdatedListeners = PurchasesUpdatedListener { billingResult, purchases ->
        Log.v(TAG, "PurchasesUpdatedListener: $billingResult")
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

        Log.v(TAG, "Connect to Google Play")
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

    private suspend fun handleBillingApiCall(block: suspend () -> JSONObject): JSONObject =
        try {
            block()
        } catch (e: BillingException) {
            if (e.isCanceled) {
                Log.w(TAG, "Billing canceled")
                JSONObject().put("result", RESULT_CANCELED)
            } else {
                Log.e(TAG, "Billing error: $e")
                JSONObject()
                    .put("result", RESULT_ERROR)
                    .put("errorCode", e.errorCode)
            }
        } catch (_: CancellationException) {
            Log.w(TAG, "Billing coroutine canceled")
            JSONObject().put("result", RESULT_CANCELED)
        }

    suspend fun getSubscriptionPlans(): JSONObject = handleBillingApiCall {
        Log.v(TAG, "Get subscription plans")

        val productDetailsParams = Product.newBuilder()
            .setProductId("premium")
            .setProductType(ProductType.SUBS)
            .build()

        val queryProductDetailsParams = QueryProductDetailsParams.newBuilder()
            .setProductList(listOf(productDetailsParams))
            .build()

        val result = withContext(Dispatchers.IO) {
            billingClient.queryProductDetails(queryProductDetailsParams)
        }

        if (!result.billingResult.isOk) {
            Log.e(TAG, "Failed to get subscription plans: ${result.billingResult}")
            throw BillingException(result.billingResult)
        }

        Log.v(TAG, "Subscription plans:\n${result.productDetailsList}")

        val resultJson = JSONObject().put("result", RESULT_OK)

        val productArray = JSONArray().also { resultJson.put("products", it) }
        result.productDetailsList?.forEach {
            val product = JSONObject().also { productArray.put(it) }
            product.put("productId", it.productId)
            product.put("name", it.name)
            val offers = JSONArray().also { product.put("offers", it) }
            it.subscriptionOfferDetails?.forEach {
                val offer = JSONObject().also { offers.put(it) }
                offer.put("basePlanId", it.basePlanId)
                offer.put("offerId", it.offerId)
                offer.put("offerToken", it.offerToken)
                val pricingPhases = JSONArray().also { offer.put("pricingPhases", it) }
                it.pricingPhases.pricingPhaseList.forEach {
                    val pricingPhase = JSONObject().also { pricingPhases.put(it) }
                    pricingPhase.put("billingCycleCount", it.billingCycleCount)
                    pricingPhase.put("billingPeriod", it.billingPeriod)
                    pricingPhase.put("formatedPrice", it.formattedPrice)
                    pricingPhase.put("recurrenceMode", it.recurrenceMode)
                }
            }
        }
        resultJson
    }

    suspend fun getCustomerCountryCode(): JSONObject = handleBillingApiCall {
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

        JSONObject()
            .put("result", RESULT_OK)
            .put("countryCode", countryCode)
    }

    suspend fun purchaseSubscription(activity: Activity, obfuscatedAccountId: String): JSONObject =
        handleBillingApiCall {
            Log.v(TAG, "Purchase subscription")
            billingClient.launchBillingFlow(activity, BillingFlowParams.newBuilder()
                .setObfuscatedAccountId(obfuscatedAccountId)
                .build())
            JSONObject()
        }

    override fun close() {
        Log.d(TAG, "Close billing client connection")
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
