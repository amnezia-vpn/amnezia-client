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

    override suspend fun purchaseSubscription(activity: Activity): String = withBillingProvider(context) {
        purchaseSubscription(activity, "obfuscatedAccountId")
    }
}
