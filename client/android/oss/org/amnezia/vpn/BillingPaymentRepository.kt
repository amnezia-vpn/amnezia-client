package org.amnezia.vpn

import android.app.Activity
import android.content.Context

class BillingPaymentRepository(@Suppress("UNUSED_PARAMETER") context: Context) : BillingRepository {
    override suspend fun getCountryCode(): String = ""
    override suspend fun getSubscriptionPlans(): String = ""
    override suspend fun purchaseSubscription(activity: Activity, offerToken: String): String = ""
    override suspend fun upgradeSubscription(activity: Activity, offerToken: String, oldPurchaseToken: String): String = ""
    override suspend fun acknowledge(purchaseToken: String): String = ""
    override suspend fun queryPurchases(): String = ""
}
