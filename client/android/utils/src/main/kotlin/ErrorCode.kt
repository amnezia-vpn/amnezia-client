package org.amnezia.vpn.util

// keep synchronized with client/core/defs.h error_code_ns::ErrorCode
object ErrorCode {
    const val NoError = 0

    const val BillingCanceled = 1300
    const val BillingError = 1301
    const val BillingGooglePlayError = 1302
    const val BillingUnavailable = 1303
    const val SubscriptionAlreadyOwned = 1304
    const val SubscriptionUnavailable = 1305
    const val BillingNetworkError = 1306
}
