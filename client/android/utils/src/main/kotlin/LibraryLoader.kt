package org.amnezia.vpn.util

import android.annotation.SuppressLint
import android.content.Context
import android.os.Build
import java.io.File
import java.io.FileOutputStream
import java.util.zip.ZipFile

private const val TAG = "LibraryLoader"

object LibraryLoader {
    private fun extractLibrary(context: Context, libraryName: String, destination: File): Boolean {
        Log.d(TAG, "Extracting library: $libraryName")
        val apks = hashSetOf<String>()
        context.applicationInfo.run {
            sourceDir?.let { apks += it }
            splitSourceDirs?.let { apks += it }
        }
        for (abi in Build.SUPPORTED_ABIS) {
            for (apk in apks) {
                ZipFile(File(apk), ZipFile.OPEN_READ).use { zipFile ->
                    val mappedName = System.mapLibraryName(libraryName)
                    val libraryZipPath = listOf("lib", abi, mappedName).joinToString(File.separator)
                    val zipEntry = zipFile.getEntry(libraryZipPath)
                    zipEntry?.let {
                        Log.d(TAG, "Extracting apk:/$libraryZipPath to ${destination.absolutePath}")
                        FileOutputStream(destination).use { outStream ->
                            zipFile.getInputStream(zipEntry).use { inStream ->
                                inStream.copyTo(outStream, 32 * 1024)
                                outStream.fd.sync()
                            }
                        }
                    }
                    return true
                }
            }
        }
        return false
    }

    @SuppressLint("UnsafeDynamicallyLoadedCode")
    fun loadSharedLibrary(context: Context, libraryName: String) {
        Log.d(TAG, "Loading library: $libraryName")
        try {
            System.loadLibrary(libraryName)
            return
        } catch (_: UnsatisfiedLinkError) {
            Log.w(TAG, "Failed to load library, try to extract it from apk")
        }
        var tempFile: File? = null
        try {
            tempFile = File.createTempFile("lib", ".so", context.codeCacheDir)
            if (extractLibrary(context, libraryName, tempFile)) {
                System.load(tempFile.absolutePath)
                return
            }
        } catch (e: Exception) {
            throw LoadLibraryException("Failed to load library apk: $libraryName", e)
        } finally {
            tempFile?.delete()
        }
    }
}

class LoadLibraryException(message: String? = null, cause: Throwable? = null) : Exception(message, cause)
