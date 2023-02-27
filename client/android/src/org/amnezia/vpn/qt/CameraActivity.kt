package org.amnezia.vpn.qt

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.content.Intent
import android.content.pm.PackageManager
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.camera.core.*
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.content.ContextCompat
import com.google.mlkit.vision.barcode.BarcodeScannerOptions
import com.google.mlkit.vision.barcode.BarcodeScanning
import com.google.mlkit.vision.barcode.common.Barcode
import com.google.mlkit.vision.common.InputImage
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import org.amnezia.vpn.R


class CameraActivity : AppCompatActivity() {

    private val CAMERA_REQUEST = 100

    private lateinit var cameraExecutor: ExecutorService
    private lateinit var analyzerExecutor: ExecutorService

    private lateinit var viewFinder: PreviewView

    companion object {
        private lateinit var instance: CameraActivity

        @JvmStatic fun getInstance(): CameraActivity {
            return instance
        }

        @JvmStatic fun stopQrCodeReader() {
            CameraActivity.getInstance().finish()
        }
    }

    external fun passDataToDecoder(data: String)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_camera)

        viewFinder = findViewById(R.id.viewFinder)

        cameraExecutor = Executors.newSingleThreadExecutor()
        analyzerExecutor = Executors.newSingleThreadExecutor()

        instance = this

        checkPermissions()

        configureVideoPreview()
    }

    private fun checkPermissions() {
        if (checkSelfPermission(Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(arrayOf(Manifest.permission.CAMERA), CAMERA_REQUEST)
        }
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == CAMERA_REQUEST) {
            if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this, "CameraX permission granted", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(this, "CameraX permission denied", Toast.LENGTH_SHORT).show();
            }
        }
    }

    @SuppressLint("UnsafeOptInUsageError")
    private fun configureVideoPreview() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)
        val imageCapture = ImageCapture.Builder().build()

        cameraProviderFuture.addListener({
            val cameraProvider: ProcessCameraProvider = cameraProviderFuture.get()

            val preview = Preview.Builder().build()

            val cameraSelector = CameraSelector.DEFAULT_BACK_CAMERA

            val imageAnalyzer = BarCodeAnalyzer()

            val analysisUseCase = ImageAnalysis.Builder()
                .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                .build()

            analysisUseCase.setAnalyzer(analyzerExecutor, imageAnalyzer)

            try {
                preview.setSurfaceProvider(viewFinder.surfaceProvider)
                cameraProvider.unbindAll()
                cameraProvider.bindToLifecycle(this, cameraSelector, preview, imageCapture, analysisUseCase)
            } catch(exc: Exception) {
                Log.e("WUTT", "Use case binding failed", exc)
            }
        }, ContextCompat.getMainExecutor(this))
    }

    override fun onDestroy() {
        cameraExecutor.shutdown()
        analyzerExecutor.shutdown()

        super.onDestroy()
    }

    val barcodesSet = mutableSetOf<String>()

    private inner class BarCodeAnalyzer(): ImageAnalysis.Analyzer {

        private val options = BarcodeScannerOptions.Builder()
            .setBarcodeFormats(Barcode.FORMAT_QR_CODE)
            .build()

        private val scanner = BarcodeScanning.getClient(options)

        @SuppressLint("UnsafeOptInUsageError")
        override fun analyze(imageProxy: ImageProxy) {
            val mediaImage = imageProxy.image

            if (mediaImage != null) {
                val image = InputImage.fromMediaImage(mediaImage, imageProxy.imageInfo.rotationDegrees)

                scanner.process(image)
                    .addOnSuccessListener { barcodes ->
                        if (barcodes.isNotEmpty()) {
                            val barcode = barcodes[0]
                            if (barcode != null) {
                                val str = barcode?.displayValue ?: ""
                                if (str.isNotEmpty()) {
                                    val isAdded = barcodesSet.add(str)
                                    if (isAdded) {
                                        passDataToDecoder(str)
                                    }
                                }
                            }
                        }

                        imageProxy.close()
                    }
                    .addOnFailureListener {
                        imageProxy.close()
                    }
            }
        }
    }
}