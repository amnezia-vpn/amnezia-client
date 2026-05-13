package org.amnezia.vpn

import android.graphics.Rect
import android.graphics.RectF
import com.google.mlkit.vision.barcode.common.Barcode
import kotlin.math.floor
import kotlin.math.max
import kotlin.math.min

object PairingQrScanGeometry {
    fun viewRectToInputImageRectFillCenter(
        viewW: Int,
        viewH: Int,
        imageW: Int,
        imageH: Int,
        viewRect: RectF
    ): RectF {
        val scale = max(viewW / imageW.toFloat(), viewH / imageH.toFloat())
        val drawLeft = (viewW - imageW * scale) / 2f
        val drawTop = (viewH - imageH * scale) / 2f
        return RectF(
            (viewRect.left - drawLeft) / scale,
            (viewRect.top - drawTop) / scale,
            (viewRect.right - drawLeft) / scale,
            (viewRect.bottom - drawTop) / scale
        )
    }

    fun pairingIosStyleHoleCornerRadiusPx(sidePx: Float, density: Float): Float {
        val d = density
        var holeR = min(28f * d, max(10f * d, sidePx * 0.056f))
        val half = 0.5f * sidePx
        holeR = min(holeR, max(6f * d, half - 2f * d))
        return max(holeR, 1f)
    }

    fun barcodeBoxOverlapFraction(roi: RectF, box: Rect): Float {
        val bf = RectF(box)
        val inter = RectF(roi)
        if (!inter.intersect(bf)) return 0f
        val interArea = inter.width() * inter.height()
        val boxArea = bf.width() * bf.height()
        return if (boxArea <= 0f) 0f else interArea / boxArea
    }

    fun barcodeMatchesPairingHole(
        roiInImageSpace: RectF,
        imageW: Int,
        imageH: Int,
        barcode: Barcode,
        minOverlapFraction: Float = PAIRING_SEND_MIN_OVERLAP_BBOX_FALLBACK
    ): Boolean {
        if (imageW <= 0 || imageH <= 0) {
            return false
        }
        val roi = RectF(roiInImageSpace)
        val iw = imageW.toFloat()
        val ih = imageH.toFloat()
        roi.left = max(0f, roi.left)
        roi.top = max(0f, roi.top)
        roi.right = min(iw, roi.right)
        roi.bottom = min(ih, roi.bottom)
        if (roi.width() <= 0f || roi.height() <= 0f) {
            return false
        }

        val corners = barcode.cornerPoints
        if (corners != null && corners.size >= 4) {
            for (p in corners) {
                if (!roi.contains(p.x.toFloat(), p.y.toFloat())) {
                    return false
                }
            }
            return true
        }

        val box = barcode.boundingBox ?: return false
        val cx = box.centerX().toFloat()
        val cy = box.centerY().toFloat()
        if (!roi.contains(cx, cy)) {
            return false
        }
        return barcodeBoxOverlapFraction(roi, box) >= minOverlapFraction
    }

    private const val PAIRING_SEND_MIN_OVERLAP_BBOX_FALLBACK = 0.72f

    fun pairingIosStyleHoleRectF(
        viewW: Int,
        viewH: Int,
        headerBottomPx: Float,
        statusBarTopPx: Float,
        density: Float
    ): RectF {
        val w = viewW.toFloat()
        val h = viewH.toFloat()
        val d = density
        if (w < 32f || h < 32f) {
            return RectF()
        }
        var hdrBottom = headerBottomPx
        if (hdrBottom < 8f * d) {
            hdrBottom = 132f * d + statusBarTopPx
        }
        val sqSz = floor(min(w, h) * 0.72).toFloat()
        var sqX = (w - sqSz) / 2f
        var sqY = (h - sqSz) / 2f
        sqY = max(sqY, hdrBottom + 8f * d)
        val kBottomBand = 80f * d
        val maxHoleBottom = h - kBottomBand
        if (sqY + sqSz > maxHoleBottom) {
            sqY = maxHoleBottom - sqSz
            sqY = max(sqY, hdrBottom + 8f * d)
        }
        sqX = max(8f * d, min(sqX, w - sqSz - 8f * d))
        sqY = max(hdrBottom + 4f * d, min(sqY, h - sqSz - 8f * d))
        return RectF(sqX, sqY, sqX + sqSz, sqY + sqSz)
    }

    fun pairingIosStyleTorchCenterYPx(
        holeBottomPx: Float,
        bandBottomPx: Float,
        headerBottomPx: Float,
        safeBottomPx: Float,
        density: Float
    ): Float {
        val torchH = 56f * density
        val d = density
        var torchCy = (holeBottomPx + bandBottomPx) * 0.5f
        val minC = holeBottomPx + torchH * 0.5f + 6f * d
        val maxC = bandBottomPx - torchH * 0.5f - max(6f * d, safeBottomPx)
        torchCy = max(minC, min(maxC, torchCy))
        if (minC > maxC) {
            torchCy = (minC + maxC) * 0.5f
        }
        val hdr = headerBottomPx + torchH * 0.5f + 10f * d
        return max(torchCy, hdr)
    }

    fun pairingIosStyleHoleInImageCoords(
        viewW: Int,
        viewH: Int,
        headerBottomPx: Float,
        statusBarTopPx: Float,
        density: Float,
        imageW: Int,
        imageH: Int
    ): RectF {
        val hv = pairingIosStyleHoleRectF(viewW, viewH, headerBottomPx, statusBarTopPx, density)
        return viewRectToInputImageRectFillCenter(viewW, viewH, imageW, imageH, hv)
    }
}
