package org.amnezia.vpn.util

// keep synchronized with client/core/defs.h error_code_ns::ErrorCode
object ErrorCode {
    const val BillingError = 1300
    const val BillingGooglePlayError = 1301
    const val BillingUnavailable = 1302
    const val SubscriptionAlreadyOwned = 1303
    const val SubscriptionUnavailable = 1304
    const val BillingNetworkError = 1305
}
