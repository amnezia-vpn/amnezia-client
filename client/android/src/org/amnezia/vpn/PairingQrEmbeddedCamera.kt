package org.amnezia.vpn

import android.graphics.Color
import android.graphics.drawable.ColorDrawable
import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.camera.core.Camera
import androidx.camera.core.CameraSelector
import androidx.camera.core.ExperimentalGetImage
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.core.content.ContextCompat
import com.google.mlkit.vision.barcode.BarcodeScanning
import com.google.mlkit.vision.barcode.BarcodeScannerOptions.Builder
import com.google.mlkit.vision.barcode.ZoomSuggestionOptions
import com.google.mlkit.vision.barcode.common.Barcode
import com.google.mlkit.vision.common.InputImage
import org.amnezia.vpn.qt.QtAndroidController
import org.amnezia.vpn.util.Log

private const val TAG = "PairingQrEmbedded"

@OptIn(ExperimentalGetImage::class)
class PairingQrEmbeddedCamera(private val activity: AmneziaActivity) {

    private var previewView: PreviewView? = null
    private var cameraProvider: ProcessCameraProvider? = null
    private var boundCamera: Camera? = null
    private val checkedBarcodes = hashSetOf<String>()
    private var barcodeScanner: com.google.mlkit.vision.barcode.BarcodeScanner? = null

    private val windowBgOpaque = ColorDrawable(Color.parseColor("#0E0E11"))

    fun start() {
        activity.runOnUiThread {
            if (previewView != null) {
                return@runOnUiThread
            }
            checkedBarcodes.clear()
            activity.window.setBackgroundDrawable(ColorDrawable(Color.TRANSPARENT))

            val content = activity.findViewById<ViewGroup>(android.R.id.content)
            val pv = PreviewView(activity).apply {
                layoutParams = FrameLayout.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT,
                    ViewGroup.LayoutParams.MATCH_PARENT
                )
                implementationMode = PreviewView.ImplementationMode.COMPATIBLE
                scaleType = PreviewView.ScaleType.FILL_CENTER
            }
            content.addView(pv, 0)
            for (i in 1 until content.childCount) {
                content.getChildAt(i).bringToFront()
            }
            previewView = pv

            val cameraProviderFuture = ProcessCameraProvider.getInstance(activity)
            cameraProviderFuture.addListener({
                try {
                    cameraProvider = cameraProviderFuture.get()
                    bindUseCases(pv)
                } catch (e: Exception) {
                    Log.e(TAG, "Camera bind failed: $e")
                    stop()
                }
            }, ContextCompat.getMainExecutor(activity))
        }
    }

    private fun bindUseCases(viewFinder: PreviewView) {
        val provider = cameraProvider ?: return
        provider.unbindAll()

        val preview = Preview.Builder().build().also {
            it.setSurfaceProvider(viewFinder.surfaceProvider)
        }

        val imageAnalysis = ImageAnalysis.Builder()
            .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
            .build()

        val cam = provider.bindToLifecycle(
            activity,
            CameraSelector.DEFAULT_BACK_CAMERA,
            preview,
            imageAnalysis
        )
        boundCamera = cam

        barcodeScanner = BarcodeScanning.getClient(
            Builder()
                .setBarcodeFormats(Barcode.FORMAT_QR_CODE)
                .setZoomSuggestionOptions(
                    ZoomSuggestionOptions.Builder { zoomLevel ->
                        cam.cameraControl.setZoomRatio(zoomLevel)
                        true
                    }.apply {
                        cam.cameraInfo.zoomState.value?.maxZoomRatio?.let { maxZoom ->
                            setMaxSupportedZoomRatio(maxZoom)
                        }
                    }.build()
                ).build()
        )

        val scanner = barcodeScanner!!
        imageAnalysis.setAnalyzer(ContextCompat.getMainExecutor(activity)) { imageProxy ->
            val mediaImage = imageProxy.image ?: run {
                imageProxy.close()
                return@setAnalyzer
            }
            val image = InputImage.fromMediaImage(mediaImage, imageProxy.imageInfo.rotationDegrees)
            scanner.process(image).addOnSuccessListener { barcodes ->
                barcodes.firstOrNull()?.displayValue?.let { code ->
                    if (code.isNotEmpty() && code !in checkedBarcodes) {
                        checkedBarcodes.add(code)
                        if (QtAndroidController.decodeQrCode(code)) {
                            scanner.close()
                            barcodeScanner = null
                            stop()
                        }
                    }
                }
            }.addOnFailureListener {
                Log.e(TAG, "QR process failed: ${it.message}")
            }.addOnCompleteListener {
                imageProxy.close()
            }
        }
    }

    fun setTorch(on: Boolean) {
        activity.runOnUiThread {
            try {
                boundCamera?.cameraControl?.enableTorch(on)
            } catch (e: Exception) {
                Log.e(TAG, "Torch: $e")
            }
        }
    }

    fun stop() {
        activity.runOnUiThread {
            try {
                boundCamera?.cameraControl?.enableTorch(false)
            } catch (_: Exception) {
            }
            boundCamera = null
            barcodeScanner?.close()
            barcodeScanner = null
            cameraProvider?.unbindAll()
            cameraProvider = null

            previewView?.let { pv ->
                (pv.parent as? ViewGroup)?.removeView(pv)
            }
            previewView = null

            activity.window.setBackgroundDrawable(windowBgOpaque)
        }
    }
}
