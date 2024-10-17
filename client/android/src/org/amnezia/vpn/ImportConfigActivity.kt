package org.amnezia.vpn

import android.Manifest
import android.content.Intent
import android.content.Intent.ACTION_SEND
import android.content.Intent.ACTION_VIEW
import android.content.Intent.CATEGORY_DEFAULT
import android.content.Intent.EXTRA_TEXT
import android.content.Intent.FLAG_ACTIVITY_NEW_TASK
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build.VERSION
import android.os.Build.VERSION_CODES
import android.os.Bundle
import android.os.Process
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts.RequestPermission
import java.io.BufferedReader
import java.io.IOException
import org.amnezia.vpn.util.Log

private const val TAG = "ImportConfigActivity"

const val ACTION_IMPORT_CONFIG = "org.amnezia.vpn.IMPORT_CONFIG"
const val EXTRA_CONFIG = "CONFIG"

class ImportConfigActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.v(TAG, "Create Import Config Activity: $intent")
        intent?.let(::readConfig)
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        Log.v(TAG, "onNewIntent: $intent")
        intent.let(::readConfig)
    }

    private fun readConfig(intent: Intent) {
        when (intent.action) {
            ACTION_SEND -> {
                Log.v(TAG, "Process SEND action, type: ${intent.type}")
                when (intent.type) {
                    "application/octet-stream" -> {
                        intent.getUriCompat()?.let { uri ->
                            checkPermissions(
                                uri,
                                onSuccess = ::processUri,
                                onFail = ::finish
                            )
                        }
                        return
                    }

                    "text/plain" -> intent.getStringExtra(EXTRA_TEXT)?.let(::startMainActivity)
                }
            }

            ACTION_VIEW -> {
                Log.v(TAG, "Process VIEW action, scheme: ${intent.scheme}")
                when (intent.scheme) {
                    "file", "content" -> {
                        intent.data?.let { uri ->
                            checkPermissions(
                                uri,
                                onSuccess = ::processUri,
                                onFail = ::finish
                            )
                        }
                        return
                    }

                    "vpn" -> intent.data?.toString()?.let(::startMainActivity)
                }

            }
        }
        finish()
    }

    private fun checkPermissions(uri: Uri, onSuccess: (Uri) -> Unit, onFail: () -> Unit) {
        if (checkUriReadPermission(uri) == PackageManager.PERMISSION_GRANTED) {
            onSuccess(uri)
        } else {
            val requestPermissionLauncher =
                registerForActivityResult(RequestPermission()) { isGranted ->
                    if (isGranted) onSuccess(uri)
                    else {
                        Toast.makeText(this, "Permission denied", Toast.LENGTH_SHORT).show()
                        onFail()
                    }
                }
            requestPermissionLauncher.launch(Manifest.permission.READ_EXTERNAL_STORAGE)
        }
    }

    private fun checkUriReadPermission(uri: Uri) = checkUriPermission(
        uri,
        Manifest.permission.READ_EXTERNAL_STORAGE,
        null,
        Process.myPid(),
        Process.myUid(),
        Intent.FLAG_GRANT_READ_URI_PERMISSION
    )

    private fun processUri(uri: Uri) {
        try {
            contentResolver.openInputStream(uri)?.use {
                it.bufferedReader().use(BufferedReader::readText).let(::startMainActivity)
            }
        } catch (e: IOException) {
            e.printStackTrace()
        } finally {
            finish()
        }
    }

    private fun Intent.getUriCompat(): Uri? =
        if (VERSION.SDK_INT >= VERSION_CODES.TIRAMISU) {
            getParcelableExtra(Intent.EXTRA_STREAM, Uri::class.java)
        } else {
            @Suppress("DEPRECATION")
            getParcelableExtra(Intent.EXTRA_STREAM)
        }

    private fun startMainActivity(config: String) {
        if (config.isNotBlank()) {
            Log.d(TAG, "startMainActivity")
            Intent(applicationContext, AmneziaActivity::class.java).apply {
                action = ACTION_IMPORT_CONFIG
                addCategory(CATEGORY_DEFAULT)
                putExtra(EXTRA_CONFIG, config)
                flags = FLAG_ACTIVITY_NEW_TASK
            }.also {
                startActivity(it)
            }
        }
    }
}
