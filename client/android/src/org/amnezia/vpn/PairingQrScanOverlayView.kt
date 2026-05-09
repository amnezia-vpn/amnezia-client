package org.amnezia.vpn

import android.content.Context
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.Path
import android.graphics.RectF
import android.util.AttributeSet
import android.view.MotionEvent
import android.view.View
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import kotlin.math.max

/**
 * iOS-style pairing scan chrome: dim outside a central square + white corner brackets.
 * Hole layout matches iOS `layoutScanOverlayGeometry` ([pairingIosStyleHoleRectF]).
 */
class PairingQrScanOverlayView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    init {
        isClickable = false
        isFocusable = false
    }

    /** Let taps reach [PreviewView] below for tap-to-focus. */
    @Suppress("ClickableViewAccessibility")
    override fun onTouchEvent(event: MotionEvent): Boolean = false

    private val dimPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = 0x8C000000.toInt() // ~55% black, same role as QML dimAlpha 0.55
        style = Paint.Style.FILL
    }

    private val bracketPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = 0xFFE8E8EC.toInt()
        style = Paint.Style.STROKE
        strokeCap = Paint.Cap.ROUND
        strokeJoin = Paint.Join.ROUND
    }

    private var hole = RectF()

    private val bracketPaths = arrayOfNulls<Path>(4)

    private var pairingHeaderBottomPx = 0f

    fun setPairingHeaderBottomPx(px: Float) {
        if (pairingHeaderBottomPx == px) {
            return
        }
        pairingHeaderBottomPx = px
        recomputePairingHole()
        invalidate()
    }

    private fun recomputePairingHole() {
        val w = width
        val h = height
        if (w <= 0 || h <= 0) {
            return
        }
        val topInset = ViewCompat.getRootWindowInsets(this)
            ?.getInsets(WindowInsetsCompat.Type.statusBars())?.top?.toFloat() ?: 0f
        val d = resources.displayMetrics.density
        hole = PairingQrScanGeometry.pairingIosStyleHoleRectF(w, h, pairingHeaderBottomPx, topInset, d)
        rebuildBracketPaths()
    }

    private fun rebuildBracketPaths() {
        val s = hole.width()
        if (s <= 0f) {
            bracketPaths.fill(null)
            return
        }
        val x0 = hole.left
        val y0 = hole.top
        val t = bracketPaint.strokeWidth
        val d = resources.displayMetrics.density
        val l = max(28f * d, s * 0.13f)
        val r = PairingQrScanGeometry.pairingIosStyleHoleCornerRadiusPx(s, d)
        for (i in 0..3) {
            bracketPaths[i] = PairingQrScanBracketPaths.bracketStrokePath(i, x0, y0, s, r, l, t)
        }
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        bracketPaint.strokeWidth = max(3f, 5f * resources.displayMetrics.density)
        recomputePairingHole()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val w = width.toFloat()
        val h = height.toFloat()
        val holeLeft = hole.left
        val holeTop = hole.top
        val holeRight = hole.right
        val holeBottom = hole.bottom

        // Four dim rects (same idea as QML dimLayer)
        canvas.drawRect(0f, 0f, w, holeTop, dimPaint)
        canvas.drawRect(0f, holeBottom, w, h, dimPaint)
        canvas.drawRect(0f, holeTop, holeLeft, holeBottom, dimPaint)
        canvas.drawRect(holeRight, holeTop, w, holeBottom, dimPaint)

        for (i in 0..3) {
            bracketPaths[i]?.let { canvas.drawPath(it, bracketPaint) }
        }
    }
}
