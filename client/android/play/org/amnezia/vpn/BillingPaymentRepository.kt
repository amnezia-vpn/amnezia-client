package org.amnezia.vpn

import android.app.Activity
import android.content.Context
import BillingProvider.Companion.withBillingProvider

class BillingPaymentRepository(private val context: Context) : BillingRepository {

    override suspend fun getCountryCode(): String = withBillingProvider(context) {
        getCustomerCountryCode()
    }

    override suspend fun getSubscriptionPlans(): String = withBillingProvider(context) {
        getSubscriptionPlans()
    }

    override suspend fun purchaseSubscription(activity: Activity, offerToken: String): String =
        withBillingProvider(context) {
            purchaseSubscription(activity, offerToken)
    }

    override suspend fun upgradeSubscription(activity: Activity, offerToken: String, oldPurchaseToken: String): String =
        withBillingProvider(context) {
            purchaseSubscription(activity, offerToken, oldPurchaseToken)
        }

    override suspend fun acknowledge(purchaseToken: String): String = withBillingProvider(context) {
        acknowledge(purchaseToken)
    }

    override suspend fun queryPurchases(): String = withBillingProvider(context) {
        getPurchases()
    }
}
