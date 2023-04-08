package org.amnezia.vpn.qt

import android.Manifest
import android.app.Activity
import android.content.pm.PackageManager
import android.content.ContentResolver
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.MediaStore
import android.widget.Toast
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

import java.io.*

import org.amnezia.vpn.R


const val INTENT_ACTION_IMPORT_CONFIG = "org.amnezia.vpn.qt.IMPORT_CONFIG"

class ImportConfigActivity : Activity() {

    private val STORAGE_PERMISSION_CODE = 42

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_import_config)
        startReadConfig(intent)
    }

    override fun onNewIntent(intent: Intent?) {
        super.onNewIntent(intent)
        startReadConfig(intent)
    }

    private fun startMainActivity(config: String?) {

        if (config == null || config.length == 0)  {
            return
        }

        val activityIntent = Intent(applicationContext, VPNActivity::class.java)
        activityIntent.action = INTENT_ACTION_IMPORT_CONFIG
        activityIntent.addCategory("android.intent.category.DEFAULT")
        activityIntent.putExtra("CONFIG", config)

        startActivity(activityIntent)
        finish()
    }

    private fun startReadConfig(intent: Intent?) {
        val newIntent = intent
        val newIntentAction: String = newIntent?.action ?: ""

        if (newIntent != null && newIntentAction == Intent.ACTION_VIEW) {
            readConfig(newIntent, newIntentAction)
        }
    }

    private fun readConfig(newIntent: Intent, newIntentAction: String) {
        if (isReadStorageAllowed()) {
            val configString = processIntent(newIntent, newIntentAction)
            startMainActivity(configString)
        } else {
            requestStoragePermission()
        }
    }

    private fun requestStoragePermission() {
        ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE), STORAGE_PERMISSION_CODE)
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String?>, grantResults: IntArray) {
        if (requestCode == STORAGE_PERMISSION_CODE) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                val configString = processIntent(intent, intent.action!!)

                if (configString != null) {
                    startMainActivity(configString)
                }
            } else {
                Toast.makeText(this, "Oops you just denied the permission", Toast.LENGTH_LONG).show()
            }
        }
    }

    private fun processIntent(intent: Intent, action: String): String? {
        val scheme = intent.scheme

        if (scheme == null) {
            return null
        }

        if (action.compareTo(Intent.ACTION_VIEW) == 0) {
            val resolver = contentResolver

            if (scheme.compareTo(ContentResolver.SCHEME_CONTENT) == 0) {
                val uri = intent.data
                val name: String? = getContentName(resolver, uri)

                println("Content intent detected: " + action + " : " + intent.dataString + " : " + intent.type + " : " + name)

                val input = resolver.openInputStream(uri!!)

                return input?.bufferedReader()?.use(BufferedReader::readText)
            } else if (scheme.compareTo(ContentResolver.SCHEME_FILE) == 0) {
                val uri = intent.data
                val name = uri!!.lastPathSegment

                println("File intent detected: " + action + " : " + intent.dataString + " : " + intent.type + " : " + name)

                val input = resolver.openInputStream(uri)

                return input?.bufferedReader()?.use(BufferedReader::readText)
            }
        }

        return null
    }

    private fun isReadStorageAllowed(): Boolean {
        val permissionStatus = ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE)
        return permissionStatus == PackageManager.PERMISSION_GRANTED
    }

    private fun getContentName(resolver: ContentResolver?, uri: Uri?): String? {
        val cursor = resolver!!.query(uri!!, null, null, null, null)

        cursor.use {
            cursor!!.moveToFirst()
            val nameIndex = cursor.getColumnIndex(MediaStore.MediaColumns.DISPLAY_NAME)
            return if (nameIndex >= 0) {
                return cursor.getString(nameIndex)
            } else {
                null
            }
        }
    }
}