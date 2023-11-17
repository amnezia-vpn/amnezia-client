package org.amnezia.vpn

import android.content.Intent
import android.net.Uri
import java.io.FileNotFoundException
import java.io.FileOutputStream
import java.io.IOException
import org.qtproject.qt.android.bindings.QtActivity

private const val TAG = "AmneziaActivity"

private const val CAMERA_ACTION_CODE = 101
private const val CREATE_FILE_ACTION_CODE = 102

class AmneziaActivity : QtActivity() {

    private var tmpFileContentToSave: String = ""

    private fun startQrCodeActivity() {
        val intent = Intent(this, CameraActivity::class.java)
        startActivityForResult(intent, CAMERA_ACTION_CODE)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        if (requestCode == CREATE_FILE_ACTION_CODE && resultCode == RESULT_OK) {
            data?.data?.also { uri ->
                alterDocument(uri)
            }
        }
        super.onActivityResult(requestCode, resultCode, data)
    }

    private fun alterDocument(uri: Uri) {
        try {
            applicationContext.contentResolver.openFileDescriptor(uri, "w")?.use { fd ->
                FileOutputStream(fd.fileDescriptor).use { fos ->
                    fos.write(tmpFileContentToSave.toByteArray())
                }
            }
        } catch (e: FileNotFoundException) {
            e.printStackTrace()
        } catch (e: IOException) {
            e.printStackTrace()
        }

        tmpFileContentToSave = ""
    }

    /**
     * Methods called by Qt
     */
    @Suppress("unused")
    fun qtAndroidControllerInitialized() {
        Log.v(TAG, "Qt Android controller initialized")
        Log.w(TAG, "Not yet implemented")
    }

    @Suppress("unused")
    fun start(vpnConfig: String) {
        Log.v(TAG, "Start VPN")
        Log.w(TAG, "Not yet implemented")
    }

    @Suppress("unused")
    fun stop() {
        Log.v(TAG, "Stop VPN")
        Log.w(TAG, "Not yet implemented")
    }

    @Suppress("unused")
    fun saveFile(fileName: String, data: String) {
        Log.v(TAG, "Save file $fileName")
        // todo: refactor
        tmpFileContentToSave = data

        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT).apply {
            addCategory(Intent.CATEGORY_OPENABLE)
            type = "text/*"
            putExtra(Intent.EXTRA_TITLE, fileName)
        }

        startActivityForResult(intent, CREATE_FILE_ACTION_CODE)
    }

    @Suppress("unused")
    fun setNotificationText(title: String, message: String, timerSec: Int) {
        Log.v(TAG, "Set notification text")
        Log.w(TAG, "Not yet implemented")
    }

    @Suppress("unused")
    fun cleanupLogs() {
        Log.v(TAG, "Cleanup logs")
        Log.w(TAG, "Not yet implemented")
    }

    @Suppress("unused")
    fun startQrCodeReader() {
        Log.v(TAG, "Start camera")
        startQrCodeActivity()
    }
}
