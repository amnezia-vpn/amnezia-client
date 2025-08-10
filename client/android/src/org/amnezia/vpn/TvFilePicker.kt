package org.amnezia.vpn

import android.content.ActivityNotFoundException
import android.content.Intent
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import org.amnezia.vpn.util.Log

private const val TAG = "TvFilePicker"

class TvFilePicker : ComponentActivity() {

    private val fileChooseResultLauncher = registerForActivityResult(ActivityResultContracts.GetContent()) {
        setResult(RESULT_OK, Intent().apply { data = it })
        finish()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.v(TAG, "onCreate")
        getFile()
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        Log.v(TAG, "onNewIntent")
        getFile()
    }

    private fun getFile() {
        try {
            Log.v(TAG, "getFile")
            fileChooseResultLauncher.launch("*/*")
        } catch (_: ActivityNotFoundException) {
            Log.w(TAG, "Activity not found")
            setResult(RESULT_CANCELED, Intent().apply { putExtra("activityNotFound", true) })
            finish()
        } catch (e: Exception) {
            Log.e(TAG, "Failed to get file: $e")
            setResult(RESULT_CANCELED)
            finish()
        }
    }
}
