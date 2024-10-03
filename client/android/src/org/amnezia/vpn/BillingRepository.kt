package org.amnezia.vpn

import android.app.Activity

interface BillingRepository {
    suspend fun getCountryCode(): String
    suspend fun getSubscriptionPlans(): String
    suspend fun purchaseSubscription(activity: Activity): String
}
