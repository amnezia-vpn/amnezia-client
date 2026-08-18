package org.amnezia.vpn

import android.Manifest
import android.app.AlertDialog
import android.content.ActivityNotFoundException
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import org.amnezia.vpn.util.Log
import java.io.File

private const val TAG = "TvFilePicker"
private const val READ_STORAGE_REQUEST_CODE = 1001

class TvFilePicker : ComponentActivity() {

    // SAF launcher for Android 10+ where File API is blocked by scoped storage
    private val safLauncher = registerForActivityResult(object : ActivityResultContracts.OpenDocument() {
        override fun createIntent(context: Context, input: Array<String>): Intent {
            val intent = super.createIntent(context, input)
            val activities = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                context.packageManager.queryIntentActivities(intent, PackageManager.ResolveInfoFlags.of(PackageManager.MATCH_DEFAULT_ONLY.toLong()))
            } else {
                @Suppress("DEPRECATION")
                context.packageManager.queryIntentActivities(intent, PackageManager.MATCH_DEFAULT_ONLY)
            }
            if (activities.all {
                    val name = it.activityInfo.packageName
                    name.startsWith("com.google.android.tv.frameworkpackagestubs") || name.startsWith("com.android.tv.frameworkpackagestubs")
                }) {
                throw ActivityNotFoundException()
            }
            return intent
        }
    }) { uri ->
        setResult(RESULT_OK, Intent().apply {
            data = uri
            addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        })
        finish()
    }

    private val directoryStack = ArrayDeque<File>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.v(TAG, "onCreate")
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            launchSaf()
        } else {
            checkPermissionAndBrowse()
        }
    }

    @Deprecated("Deprecated in Java")
    override fun onBackPressed() {
        navigateBack()
    }

    private fun launchSaf() {
        try {
            safLauncher.launch(arrayOf("*/*"))
        } catch (_: ActivityNotFoundException) {
            Log.w(TAG, "No SAF activity found")
            setResult(RESULT_CANCELED, Intent().apply { putExtra("activityNotFound", true) })
            finish()
        } catch (e: Exception) {
            Log.e(TAG, "SAF launch failed: $e")
            setResult(RESULT_CANCELED)
            finish()
        }
    }

    private fun checkPermissionAndBrowse() {
        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE), READ_STORAGE_REQUEST_CODE)
        } else {
            showRootDirectory()
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == READ_STORAGE_REQUEST_CODE &&
            grantResults.firstOrNull() == PackageManager.PERMISSION_GRANTED) {
            showRootDirectory()
        } else {
            setResult(RESULT_CANCELED)
            finish()
        }
    }

    private fun showRootDirectory() {
        @Suppress("DEPRECATION")
        val primaryExternal = Environment.getExternalStorageDirectory()
        val storageDir = File("/storage")
        // Pre-seed stack with /storage so Back from primary storage goes there (USB drives etc.)
        if (storageDir.exists() && storageDir.canonicalPath != primaryExternal.canonicalPath) {
            directoryStack.addLast(storageDir)
        }
        showDirectory(primaryExternal)
    }

    private fun navigateBack() {
        if (directoryStack.size > 1) {
            directoryStack.removeLast()
            val parent = directoryStack.removeLast()
            showDirectory(parent)
        } else {
            setResult(RESULT_CANCELED)
            finish()
        }
    }

    private fun showDirectory(dir: File) {
        directoryStack.addLast(dir)
        Log.v(TAG, "Showing directory: ${dir.absolutePath}")

        val entries = try {
            dir.listFiles()
                ?.sortedWith(compareBy({ !it.isDirectory }, { it.name.lowercase() }))
                ?: emptyList()
        } catch (e: Exception) {
            Log.e(TAG, "Failed to list directory: $e")
            emptyList()
        }

        val names = entries.map { if (it.isDirectory) "[${it.name}]" else it.name }.toTypedArray()

        val builder = AlertDialog.Builder(this)
            .setTitle(dir.absolutePath)

        if (entries.isEmpty()) {
            builder.setMessage("No files available")
        } else {
            builder.setItems(names) { dialog, which ->
                dialog.dismiss()
                val selected = entries[which]
                if (selected.isDirectory) {
                    showDirectory(selected)
                } else {
                    Log.v(TAG, "Selected file: ${selected.absolutePath}")
                    setResult(RESULT_OK, Intent().apply {
                        data = Uri.fromFile(selected)
                        addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                    })
                    finish()
                }
            }
        }

        if (directoryStack.size > 1) {
            builder.setNegativeButton("↑ Back") { dialog, _ ->
                dialog.dismiss()
                navigateBack()
            }
        } else {
            builder.setNegativeButton(android.R.string.cancel) { _, _ ->
                setResult(RESULT_CANCELED)
                finish()
            }
        }

        builder.setOnCancelListener {
            setResult(RESULT_CANCELED)
            finish()
        }

        builder.show()
    }
}
