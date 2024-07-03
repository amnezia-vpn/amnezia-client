package org.amnezia.vpn

import android.Manifest
import android.annotation.SuppressLint
import android.content.pm.PackageManager
import android.os.Bundle
import android.view.MotionEvent.ACTION_DOWN
import android.view.MotionEvent.ACTION_UP
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts.RequestPermission
import androidx.camera.core.CameraSelector
import androidx.camera.core.ExperimentalGetImage
import androidx.camera.core.FocusMeteringAction
import androidx.camera.core.FocusMeteringAction.FLAG_AE
import androidx.camera.core.FocusMeteringAction.FLAG_AF
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.core.content.ContextCompat
import com.google.mlkit.vision.barcode.BarcodeScannerOptions.Builder
import com.google.mlkit.vision.barcode.BarcodeScanning
import com.google.mlkit.vision.barcode.ZoomSuggestionOptions
import com.google.mlkit.vision.barcode.common.Barcode
import com.google.mlkit.vision.common.InputImage
import org.amnezia.vpn.databinding.CameraPreviewBinding
import org.amnezia.vpn.qt.QtAndroidController
import org.amnezia.vpn.util.Log

private const val TAG = "CameraActivity"

class CameraActivity : ComponentActivity() {

    private lateinit var viewBinding: CameraPreviewBinding
    private lateinit var cameraProvider: ProcessCameraProvider

    @ExperimentalGetImage
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        viewBinding = CameraPreviewBinding.inflate(layoutInflater)
        setContentView(viewBinding.root)

        checkPermissions(onSuccess = ::startCamera, onFail = ::finish)
    }

    private fun checkPermissions(onSuccess: () -> Unit, onFail: () -> Unit) {
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED) {
            onSuccess()
        } else {
            val requestPermissionLauncher =
                registerForActivityResult(RequestPermission()) { isGranted ->
                    if (isGranted) {
                        Toast.makeText(this, "Camera permission granted", Toast.LENGTH_SHORT).show()
                        onSuccess()
                    } else {
                        Toast.makeText(this, "Camera permission denied", Toast.LENGTH_SHORT).show()
                        onFail()
                    }
                }
            requestPermissionLauncher.launch(Manifest.permission.CAMERA)
        }
    }

    @ExperimentalGetImage
    private fun startCamera() {
        val cameraProviderFuture = ProcessCameraProvider.getInstance(this)

        cameraProviderFuture.addListener({
            cameraProvider = cameraProviderFuture.get()
            bindPreview()
            bindImageAnalysis()
        }, ContextCompat.getMainExecutor(this))
    }

    @SuppressLint("ClickableViewAccessibility")
    private fun bindPreview() {
        val viewFinder = viewBinding.viewFinder
        val preview = Preview.Builder().build().also {
            it.setSurfaceProvider(viewFinder.surfaceProvider)
        }

        val camera = cameraProvider.bindToLifecycle(this, CameraSelector.DEFAULT_BACK_CAMERA, preview)

        viewFinder.setOnTouchListener { _, motionEvent ->
            when (motionEvent.action) {
                ACTION_DOWN -> true
                ACTION_UP -> {
                    val point = viewFinder
                        .meteringPointFactory.createPoint(motionEvent.x, motionEvent.x)

                    val action = FocusMeteringAction
                        .Builder(point, FLAG_AF or FLAG_AE).build()

                    camera.cameraControl.startFocusAndMetering(action)
                    true
                }

                else -> false
            }
        }
    }

    @ExperimentalGetImage
    private fun bindImageAnalysis() {
        val imageAnalysis = ImageAnalysis.Builder().build()

        val camera = cameraProvider.bindToLifecycle(this, CameraSelector.DEFAULT_BACK_CAMERA, imageAnalysis)

        val barcodeScanner = BarcodeScanning.getClient(
            Builder()
                .setBarcodeFormats(Barcode.FORMAT_QR_CODE)
                .setZoomSuggestionOptions(
                    ZoomSuggestionOptions.Builder { zoomLevel ->
                        camera.cameraControl.setZoomRatio(zoomLevel)
                        true
                    }.apply {
                        camera.cameraInfo.zoomState.value?.maxZoomRatio?.let { maxZoomRation ->
                            setMaxSupportedZoomRatio(maxZoomRation)
                        }
                    }.build()
                ).build()
        )

        // optimization
        val checkedBarcodes = hashSetOf<String>()

        imageAnalysis.setAnalyzer(ContextCompat.getMainExecutor(this)) { imageProxy ->
            imageProxy.image?.let { InputImage.fromMediaImage(it, imageProxy.imageInfo.rotationDegrees) }
                ?.let { image ->
                    barcodeScanner.process(image).addOnSuccessListener { barcodes ->
                        barcodes.firstOrNull()?.let { barcode ->
                            barcode.displayValue?.let { code ->
                                if (code.isNotEmpty() && code !in checkedBarcodes) {
                                    if (QtAndroidController.decodeQrCode(code)) {
                                        barcodeScanner.close()
                                        stopCamera()
                                    }
                                    checkedBarcodes.add(code)
                                }
                            }
                        }
                    }.addOnFailureListener {
                        Log.e(TAG, "Processing QR code image failed: ${it.message}")
                    }.addOnCompleteListener {
                        imageProxy.close()
                    }
                }
        }
    }

    private fun stopCamera() {
        cameraProvider.unbindAll()
        finish()
    }
}
