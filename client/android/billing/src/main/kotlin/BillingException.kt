import com.android.billingclient.api.BillingClient.BillingResponseCode.BILLING_UNAVAILABLE
import com.android.billingclient.api.BillingClient.BillingResponseCode.DEVELOPER_ERROR
import com.android.billingclient.api.BillingClient.BillingResponseCode.ERROR
import com.android.billingclient.api.BillingClient.BillingResponseCode.FEATURE_NOT_SUPPORTED
import com.android.billingclient.api.BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED
import com.android.billingclient.api.BillingClient.BillingResponseCode.ITEM_NOT_OWNED
import com.android.billingclient.api.BillingClient.BillingResponseCode.ITEM_UNAVAILABLE
import com.android.billingclient.api.BillingClient.BillingResponseCode.NETWORK_ERROR
import com.android.billingclient.api.BillingClient.BillingResponseCode.SERVICE_DISCONNECTED
import com.android.billingclient.api.BillingClient.BillingResponseCode.SERVICE_UNAVAILABLE
import com.android.billingclient.api.BillingClient.BillingResponseCode.USER_CANCELED
import com.android.billingclient.api.BillingResult
import org.amnezia.vpn.util.ErrorCode

internal class BillingException(billingResult: BillingResult) : Exception(billingResult.debugMessage) {

    val errorCode: Int
    val isCanceled = billingResult.responseCode == USER_CANCELED

    init {
        when (billingResult.responseCode) {
            ERROR -> {
                errorCode = ErrorCode.BillingGooglePlayError
            }

            BILLING_UNAVAILABLE, SERVICE_DISCONNECTED, SERVICE_UNAVAILABLE -> {
                errorCode = ErrorCode.BillingUnavailable
            }

            DEVELOPER_ERROR, FEATURE_NOT_SUPPORTED, ITEM_NOT_OWNED -> {
                errorCode = ErrorCode.BillingError
            }

            ITEM_ALREADY_OWNED -> {
                errorCode = ErrorCode.SubscriptionAlreadyOwned
            }

            ITEM_UNAVAILABLE -> {
                errorCode = ErrorCode.SubscriptionUnavailable
            }

            NETWORK_ERROR -> {
                errorCode = ErrorCode.BillingNetworkError
            }

            else -> {
                errorCode = ErrorCode.BillingError
            }
        }
    }
}
