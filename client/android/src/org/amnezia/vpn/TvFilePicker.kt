package org.amnezia.vpn

import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import org.amnezia.vpn.util.Log

private const val TAG = "TvFilePicker"

class TvFilePicker : ComponentActivity() {

    private val fileChooseResultLauncher = registerForActivityResult(object : ActivityResultContracts.OpenDocument() {
        override fun createIntent(context: Context, input: Array<String>): Intent {
            val intent = super.createIntent(context, input)

            val activitiesToResolveIntent = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                context.packageManager.queryIntentActivities(intent, PackageManager.ResolveInfoFlags.of(PackageManager.MATCH_DEFAULT_ONLY.toLong()))
            } else {
                @Suppress("DEPRECATION")
                context.packageManager.queryIntentActivities(intent, PackageManager.MATCH_DEFAULT_ONLY)
            }
            if (activitiesToResolveIntent.all {
                    val name = it.activityInfo.packageName
                    name.startsWith("com.google.android.tv.frameworkpackagestubs") || name.startsWith("com.android.tv.frameworkpackagestubs")
                }) {
                throw ActivityNotFoundException()
            }
            return intent
        }
    }) {
        setResult(RESULT_OK, Intent().apply {
            data = it
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        })
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
            fileChooseResultLauncher.launch(arrayOf("*/*"))
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
