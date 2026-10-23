package org.amnezia.vpn

import android.app.Activity

interface BillingRepository {
    suspend fun getCountryCode(): String
    suspend fun getSubscriptionPlans(): String
    suspend fun purchaseSubscription(activity: Activity, offerToken: String): String
    suspend fun upgradeSubscription(activity: Activity, offerToken: String, oldPurchaseToken: String): String
    suspend fun acknowledge(purchaseToken: String): String
    suspend fun queryPurchases(): String
}
