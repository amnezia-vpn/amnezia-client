package org.amnezia.vpn

import android.Manifest
import android.annotation.SuppressLint
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.view.MotionEvent.ACTION_DOWN
import android.view.MotionEvent.ACTION_UP
import android.graphics.RectF
import android.view.Gravity
import android.view.View
import android.widget.FrameLayout
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.OnBackPressedCallback
import androidx.activity.result.contract.ActivityResultContracts.RequestPermission
import androidx.camera.core.Camera
import androidx.camera.core.CameraSelector
import androidx.camera.core.ExperimentalGetImage
import androidx.camera.core.FocusMeteringAction
import androidx.camera.core.FocusMeteringAction.FLAG_AE
import androidx.camera.core.FocusMeteringAction.FLAG_AF
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.camera.view.TransformExperimental
import androidx.camera.view.transform.CoordinateTransform
import androidx.camera.view.transform.ImageProxyTransformFactory
import androidx.camera.view.transform.OutputTransform
import androidx.core.content.ContextCompat
import androidx.core.view.ViewCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.lifecycle.Observer
import com.google.mlkit.vision.barcode.BarcodeScanner
import com.google.mlkit.vision.barcode.BarcodeScannerOptions.Builder
import com.google.mlkit.vision.barcode.BarcodeScanning
import com.google.mlkit.vision.barcode.common.Barcode
import com.google.mlkit.vision.common.InputImage
import org.amnezia.vpn.databinding.CameraPreviewBinding
import org.amnezia.vpn.qt.QtAndroidController
import org.amnezia.vpn.util.Log
import java.util.concurrent.ExecutorService
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicReference
import kotlin.math.roundToInt

private const val TAG = "CameraActivity"

@OptIn(TransformExperimental::class)
class CameraActivity : ComponentActivity() {

    companion object {
        const val EXTRA_PAIRING_QR_CAMERA = "org.amnezia.vpn.extra.PAIRING_QR_CAMERA"
    }

    private lateinit var viewBinding: CameraPreviewBinding
    private var cameraProvider: ProcessCameraProvider? = null
    private var boundCamera: Camera? = null
    private var boundImageAnalysis: ImageAnalysis? = null
    private var torchOn: Boolean = false

    /** CameraX analyzer thread only; ML Kit Task callbacks use [ContextCompat.getMainExecutor] so they are not rejected after [ExecutorService.shutdown]. */
    private var imageAnalysisExecutor: ExecutorService? = null

    /** After a successful decode, ignore further frames (ML Kit otherwise hits "detector is already closed"). */
    private val qrHandledOrClosing = AtomicBoolean(false)

    /**
     * Pairing mode: QR was accepted by Qt ([decodeQrCode] true) and we are finishing normally.
     * Do not apply JNI reopen cooldown on destroy — user may cancel confirm and return to scan immediately.
     */
    private var pairingQrDeliveredToQt = false

    /**
     * Pairing mode: user closed camera via back / toolbar (not a successful scan).
     * Skip JNI reopen cooldown — otherwise Qt stays on the black overlay shell until cooldown ends and a second back pops the page.
     */
    private var pairingQrUserDismissedCamera = false

    private var barcodeScanner: BarcodeScanner? = null

    /**
     * [PreviewView.getOutputTransform] is main-thread-only; the image analyzer runs on a background
     * executor. Refresh this cache whenever layout / preview geometry may change.
     */
    private val cachedPreviewOutputTransform = AtomicReference<OutputTransform?>(null)

    private var previewTransformLayoutListener: View.OnLayoutChangeListener? = null

    private var previewStreamStateObserver: Observer<PreviewView.StreamState>? = null

    @Volatile
    private var pairingGeomHeaderBottomPx = 0f

    @Volatile
    private var pairingGeomStatusBarTopPx = 0f

    @Volatile
    private var pairingGeomDensity = 1f

    @ExperimentalGetImage
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        viewBinding = CameraPreviewBinding.inflate(layoutInflater)
        setContentView(viewBinding.root)
        viewBinding.viewFinder.scaleType = PreviewView.ScaleType.FILL_CENTER

        if (intent.getBooleanExtra(EXTRA_PAIRING_QR_CAMERA, false)) {
            WindowCompat.setDecorFitsSystemWindows(window, false)
            val density = resources.displayMetrics.density
            val padH = (8 * density).toInt()
            val padTopBase = (28 * density).toInt()
            val padBottom = (12 * density).toInt()
            ViewCompat.setOnApplyWindowInsetsListener(viewBinding.pairingChrome) { v, windowInsets ->
                val bars = windowInsets.getInsets(WindowInsetsCompat.Type.statusBars())
                v.setPadding(padH, padTopBase + bars.top, (16 * density).toInt(), padBottom)
                v.post { onPairingLayoutGeometryChanged() }
                windowInsets
            }
            viewBinding.pairingScanOverlay.visibility = View.VISIBLE
            viewBinding.pairingChrome.visibility = View.VISIBLE
            viewBinding.root.addOnLayoutChangeListener { _, _, _, _, _, _, _, _, _ ->
                viewBinding.root.post { onPairingLayoutGeometryChanged() }
            }
            viewBinding.root.post {
                onPairingLayoutGeometryChanged()
                applyPairingTorchButtonChrome()
            }
        }

        viewBinding.pairingBack.setOnClickListener { releaseCameraAndFinish() }

        onBackPressedDispatcher.addCallback(
            this,
            object : OnBackPressedCallback(true) {
                override fun handleOnBackPressed() {
                    releaseCameraAndFinish()
                }
            }
        )

        viewBinding.torchButton.setOnClickListener {
            torchOn = !torchOn
            try {
                boundCamera?.cameraControl?.enableTorch(torchOn)
            } catch (e: Exception) {
                Log.e(TAG, "Torch: $e")
            }
            if (intent.getBooleanExtra(EXTRA_PAIRING_QR_CAMERA, false)) {
                applyPairingTorchButtonChrome()
            }
        }

        checkPermissions(onSuccess = ::startCamera, onFail = ::finish)
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        setIntent(intent)
        if (!intent.getBooleanExtra(EXTRA_PAIRING_QR_CAMERA, false)) {
            return
        }
        if (!::viewBinding.isInitialized) {
            return
        }
        Log.v(TAG, "onNewIntent: rebind pairing camera")
        cleanupCameraResources()
        qrHandledOrClosing.set(false)
        pairingQrDeliveredToQt = false
        pairingQrUserDismissedCamera = false
        torchOn = false
        viewBinding.pairingScanOverlay.visibility = View.VISIBLE
        viewBinding.pairingChrome.visibility = View.VISIBLE
        viewBinding.root.post {
            onPairingLayoutGeometryChanged()
            applyPairingTorchButtonChrome()
        }
        checkPermissions(onSuccess = ::startCamera, onFail = ::finish)
    }

    override fun onDestroy() {
        cleanupCameraResources()
        val pairing = intent.getBooleanExtra(EXTRA_PAIRING_QR_CAMERA, false)
        if (pairing && !pairingQrDeliveredToQt && !pairingQrUserDismissedCamera) {
            try {
                QtAndroidController.onPairingQrCameraClosed()
            } catch (t: Throwable) {
                Log.e(TAG, "onPairingQrCameraClosed: $t")
            }
        }
        super.onDestroy()
    }

    /** Idempotent: safe from back, successful decode, or process death. */
    private fun cleanupCameraResources() {
        qrHandledOrClosing.set(true)
        try {
            boundImageAnalysis?.clearAnalyzer()
        } catch (_: Exception) {
        }
        boundImageAnalysis = null
        try {
            barcodeScanner?.close()
        } catch (_: Exception) {
        }
        barcodeScanner = null
        try {
            boundCamera?.cameraControl?.enableTorch(false)
        } catch (_: Exception) {
        }
        boundCamera = null
        try {
            cameraProvider?.unbindAll()
        } catch (_: Exception) {
        }
        imageAnalysisExecutor?.let { ex ->
            try {
                ex.shutdown()
            } catch (_: Exception) {
            }
        }
        imageAnalysisExecutor = null
        previewTransformLayoutListener?.let { listener ->
            if (::viewBinding.isInitialized) {
                viewBinding.viewFinder.removeOnLayoutChangeListener(listener)
            }
        }
        previewTransformLayoutListener = null
        previewStreamStateObserver?.let { obs ->
            if (::viewBinding.isInitialized) {
                viewBinding.viewFinder.previewStreamState.removeObserver(obs)
            }
        }
        previewStreamStateObserver = null
        cachedPreviewOutputTransform.set(null)
    }

    /** Must run on the main thread (uses [PreviewView.getOutputTransform]). */
    private fun refreshCachedPreviewOutputTransform() {
        if (!::viewBinding.isInitialized) {
            return
        }
        val vf = viewBinding.viewFinder
        try {
            val out = vf.outputTransform
            cachedPreviewOutputTransform.set(out)
        } catch (t: Throwable) {
            Log.e(TAG, "refreshCachedPreviewOutputTransform: $t")
            cachedPreviewOutputTransform.set(null)
        }
    }

    /**
     * Schedules [refreshCachedPreviewOutputTransform] on the main thread (safe from any thread).
     */
    private fun scheduleCachedPreviewOutputTransformRefresh() {
        if (!::viewBinding.isInitialized) {
            return
        }
        viewBinding.viewFinder.post { refreshCachedPreviewOutputTransform() }
    }

    /**
     * iOS `layoutScanOverlayGeometry`: hole + torch vertical position; keeps ML Kit ROI aligned with overlay.
     */
    private fun onPairingLayoutGeometryChanged() {
        if (!::viewBinding.isInitialized || !intent.getBooleanExtra(EXTRA_PAIRING_QR_CAMERA, false)) {
            return
        }
        val root = viewBinding.root
        val chrome = viewBinding.pairingChrome
        val w = root.width
        val h = root.height
        if (w <= 0 || h <= 0) {
            return
        }
        val density = resources.displayMetrics.density
        val headerBottom = if (chrome.visibility == View.VISIBLE) chrome.bottom.toFloat() else 0f
        val insets = ViewCompat.getRootWindowInsets(root)
        val statusTop = insets?.getInsets(WindowInsetsCompat.Type.statusBars())?.top?.toFloat() ?: 0f
        val safeBottom = insets?.getInsets(WindowInsetsCompat.Type.systemBars())?.bottom?.toFloat() ?: 0f

        pairingGeomHeaderBottomPx = headerBottom
        pairingGeomStatusBarTopPx = statusTop
        pairingGeomDensity = density

        viewBinding.pairingScanOverlay.setPairingHeaderBottomPx(headerBottom)

        val hole = PairingQrScanGeometry.pairingIosStyleHoleRectF(w, h, headerBottom, statusTop, density)
        val torchCy = PairingQrScanGeometry.pairingIosStyleTorchCenterYPx(
            hole.bottom,
            h.toFloat(),
            headerBottom,
            safeBottom,
            density
        )
        val torchSizePx = (56f * density).roundToInt().coerceAtLeast(1)
        val topMargin = (torchCy - torchSizePx / 2f).roundToInt().coerceAtLeast(0)
        val wantGravity = Gravity.TOP or Gravity.CENTER_HORIZONTAL
        viewBinding.torchButton.post {
            if (!::viewBinding.isInitialized || !intent.getBooleanExtra(EXTRA_PAIRING_QR_CAMERA, false)) {
                return@post
            }
            val btn = viewBinding.torchButton
            val lp = btn.layoutParams as FrameLayout.LayoutParams
            if (lp.gravity == wantGravity && lp.topMargin == topMargin && lp.bottomMargin == 0) {
                return@post
            }
            lp.gravity = wantGravity
            lp.topMargin = topMargin
            lp.bottomMargin = 0
            btn.layoutParams = lp
        }
    }

    /** Matches iOS pairing overlay torch on/off chrome (background + golden border). */
    private fun applyPairingTorchButtonChrome() {
        if (!::viewBinding.isInitialized || !intent.getBooleanExtra(EXTRA_PAIRING_QR_CAMERA, false)) {
            return
        }
        val btn = viewBinding.torchButton
        val d = resources.displayMetrics.density
        val alpha = if (torchOn) (0.42f * 255f).toInt() else (0.22f * 255f).toInt()
        val bg = GradientDrawable().apply {
            shape = GradientDrawable.OVAL
            setColor(Color.argb(alpha, 255, 255, 255))
            if (torchOn) {
                setStroke((2f * d).roundToInt(), Color.rgb(255, 191, 115))
            } else {
                setStroke(0, 0)
            }
        }
        btn.background = bg
    }

    /**
     * Maps the pairing scan square from [PreviewView] pixel space into the same coordinate system
     * as ML Kit barcodes for this [imageProxy] (rotation-aware). Falls back to FILL_CENTER math
     * if the transform is not ready — never returns view-space rects as image-space.
     */
    private fun pairingHoleRectInImageSpace(
        viewFinder: PreviewView,
        imageProxy: ImageProxy,
        imageWidth: Int,
        imageHeight: Int
    ): RectF {
        val vw = viewFinder.width
        val vh = viewFinder.height
        fun geomFallback(): RectF =
            PairingQrScanGeometry.pairingIosStyleHoleInImageCoords(
                vw,
                vh,
                pairingGeomHeaderBottomPx,
                pairingGeomStatusBarTopPx,
                pairingGeomDensity,
                imageWidth,
                imageHeight
            )
        if (vw <= 0 || vh <= 0 || imageWidth <= 0 || imageHeight <= 0) {
            return geomFallback()
        }
        return try {
            val previewOut = cachedPreviewOutputTransform.get()
            if (previewOut == null) {
                geomFallback()
            } else {
                val imageFactory = ImageProxyTransformFactory().apply {
                    setUsingRotationDegrees(true)
                }
                val imageOut = imageFactory.getOutputTransform(imageProxy)
                val holeView = PairingQrScanGeometry.pairingIosStyleHoleRectF(
                    vw,
                    vh,
                    pairingGeomHeaderBottomPx,
                    pairingGeomStatusBarTopPx,
                    pairingGeomDensity
                )
                if (holeView.width() <= 0f || holeView.height() <= 0f) {
                    return geomFallback()
                }
                val hole = RectF(holeView)
                CoordinateTransform(previewOut, imageOut).mapRect(hole)
                hole
            }
        } catch (t: Throwable) {
            Log.e(TAG, "pairingHoleRectInImageSpace: $t")
            geomFallback()
        }
    }

    private fun releaseCameraAndFinish() {
        if (intent.getBooleanExtra(EXTRA_PAIRING_QR_CAMERA, false)) {
            pairingQrUserDismissedCamera = true
            try {
                QtAndroidController.onPairingQrCameraUserDismissed()
            } catch (t: Throwable) {
                Log.e(TAG, "onPairingQrCameraUserDismissed: $t")
            }
        }
        cleanupCameraResources()
        finish()
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
            bindCameraUseCases()
        }, ContextCompat.getMainExecutor(this))
    }

    @SuppressLint("ClickableViewAccessibility")
    @ExperimentalGetImage
    private fun bindCameraUseCases() {
        val provider = cameraProvider ?: return
        imageAnalysisExecutor?.shutdown()
        imageAnalysisExecutor = Executors.newSingleThreadExecutor()

        val viewFinder = viewBinding.viewFinder
        val preview = Preview.Builder().build().also {
            it.setSurfaceProvider(viewFinder.surfaceProvider)
        }

        val imageAnalysis = ImageAnalysis.Builder()
            .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
            .build()

        val camera = provider.bindToLifecycle(
            this,
            CameraSelector.DEFAULT_BACK_CAMERA,
            preview,
            imageAnalysis
        )
        boundCamera = camera
        boundImageAnalysis = imageAnalysis

        viewFinder.setOnTouchListener { _, motionEvent ->
            when (motionEvent.action) {
                ACTION_DOWN -> true
                ACTION_UP -> {
                    val point = viewFinder
                        .meteringPointFactory.createPoint(motionEvent.x, motionEvent.y)

                    val action = FocusMeteringAction
                        .Builder(point, FLAG_AF or FLAG_AE).build()

                    camera.cameraControl.startFocusAndMetering(action)
                    true
                }

                else -> false
            }
        }

        if (intent.getBooleanExtra(EXTRA_PAIRING_QR_CAMERA, false)) {
            previewTransformLayoutListener?.let { viewFinder.removeOnLayoutChangeListener(it) }
            val layoutListener = View.OnLayoutChangeListener { _, _, _, _, _, _, _, _, _ ->
                viewFinder.post {
                    scheduleCachedPreviewOutputTransformRefresh()
                    onPairingLayoutGeometryChanged()
                }
            }
            previewTransformLayoutListener = layoutListener
            viewFinder.addOnLayoutChangeListener(layoutListener)
            previewStreamStateObserver?.let { viewFinder.previewStreamState.removeObserver(it) }
            val streamObserver = Observer<PreviewView.StreamState> { state ->
                if (state == PreviewView.StreamState.STREAMING) {
                    viewFinder.post {
                        scheduleCachedPreviewOutputTransformRefresh()
                        onPairingLayoutGeometryChanged()
                    }
                }
            }
            previewStreamStateObserver = streamObserver
            viewFinder.previewStreamState.observe(this, streamObserver)
            scheduleCachedPreviewOutputTransformRefresh()
        }

        try {
            barcodeScanner?.close()
        } catch (_: Exception) {
        }
        // No ZoomSuggestionOptions: ML Kit "scanner-auto-zoom" is off-spec for pairing and skews crop vs our ROI.
        barcodeScanner = BarcodeScanning.getClient(
            Builder()
                .setBarcodeFormats(Barcode.FORMAT_QR_CODE)
                .build()
        )

        val checkedBarcodes = hashSetOf<String>()
        val analysisExecutor = imageAnalysisExecutor!!
        val mainExecutor = ContextCompat.getMainExecutor(this)
        val pairingQrMode = intent.getBooleanExtra(EXTRA_PAIRING_QR_CAMERA, false)

        imageAnalysis.setAnalyzer(analysisExecutor) { imageProxy ->
            if (qrHandledOrClosing.get()) {
                imageProxy.close()
                return@setAnalyzer
            }
            val mediaImage = imageProxy.image
            if (mediaImage == null) {
                imageProxy.close()
                return@setAnalyzer
            }
            val image = InputImage.fromMediaImage(mediaImage, imageProxy.imageInfo.rotationDegrees)
            val viewW = viewFinder.width
            val viewH = viewFinder.height
            val pairingRoi = if (pairingQrMode) {
                pairingHoleRectInImageSpace(viewFinder, imageProxy, image.width, image.height)
            } else {
                null
            }
            val scanner = barcodeScanner ?: run {
                imageProxy.close()
                return@setAnalyzer
            }
            scanner.process(image)
                .addOnSuccessListener(mainExecutor) { barcodes ->
                    if (qrHandledOrClosing.get()) {
                        return@addOnSuccessListener
                    }
                    val barcode = if (pairingQrMode) {
                        val roi = pairingRoi
                            ?: PairingQrScanGeometry.pairingIosStyleHoleInImageCoords(
                                viewW,
                                viewH,
                                pairingGeomHeaderBottomPx,
                                pairingGeomStatusBarTopPx,
                                pairingGeomDensity,
                                image.width,
                                image.height
                            )
                        barcodes.firstOrNull {
                            PairingQrScanGeometry.barcodeMatchesPairingHole(
                                roi,
                                image.width,
                                image.height,
                                it
                            )
                        }
                    } else {
                        barcodes.firstOrNull()
                    }
                    barcode?.displayValue?.let { code ->
                            if (code.isNotEmpty() && code !in checkedBarcodes) {
                                checkedBarcodes.add(code)
                                if (QtAndroidController.decodeQrCode(code)) {
                                    if (qrHandledOrClosing.compareAndSet(false, true)) {
                                        if (pairingQrMode) {
                                            pairingQrDeliveredToQt = true
                                        }
                                        stopCamera()
                                    }
                                }
                            }
                    }
                }
                .addOnFailureListener(mainExecutor) {
                    Log.e(TAG, "Processing QR code image failed: ${it.message}")
                }
                .addOnCompleteListener(mainExecutor) {
                    imageProxy.close()
                }
        }
    }

    private fun stopCamera() {
        cleanupCameraResources()
        finish()
    }
}
