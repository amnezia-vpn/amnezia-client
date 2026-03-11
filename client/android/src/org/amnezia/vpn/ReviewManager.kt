package org.amnezia.vpn

import android.app.Activity
import com.google.android.play.core.review.ReviewManagerFactory
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import org.amnezia.vpn.util.Log
import org.amnezia.vpn.util.Prefs

private const val TAG = "ReviewManager"

private const val PREFS_REVIEW_OPEN_COUNT = "REVIEW_OPEN_COUNT"
private const val REVIEW_REQUEST_OPEN_INTERVAL = 20

object ReviewManager {

    @Volatile
    private var shouldRequestReviewOnFocus = false

    /**
     * Call from onResume
     * Increments the open count and arms the flag if the interval is hit.
     * I/O runs on the IO dispatcher to avoid blocking the main thread.
     */
    fun onActivityResumed(scope: CoroutineScope) {
        scope.launch(Dispatchers.IO) {
            val openCount = Prefs.load<Int>(PREFS_REVIEW_OPEN_COUNT) + 1
            val shouldRequest = openCount >= REVIEW_REQUEST_OPEN_INTERVAL
            Prefs.save(PREFS_REVIEW_OPEN_COUNT, if (shouldRequest) 0 else openCount)

            shouldRequestReviewOnFocus = shouldRequest
            Log.i(TAG, "onActivityResumed: openCount=$openCount, shouldRequestReview=$shouldRequestReviewOnFocus")
        }
    }

    /**
     * Call from onWindowFocusChanged (hasFocus=true)
     */
    fun onWindowFocusGained(activity: Activity, scope: CoroutineScope) {
        if (!shouldRequestReviewOnFocus) return
        shouldRequestReviewOnFocus = false

        scope.launch(Dispatchers.Main) {
            requestReviewFlow(activity)
        }
    }

    private fun requestReviewFlow(activity: Activity) {
        val reviewManager = ReviewManagerFactory.create(activity)
        reviewManager.requestReviewFlow().addOnCompleteListener { request ->
            if (request.isSuccessful) {
                reviewManager.launchReviewFlow(activity, request.result)
            } else {
                Log.w(TAG, "Review flow request failed: ${request.exception}")
            }
        }
    }
}
