package org.amnezia.vpn

import android.graphics.Path
import android.graphics.RectF
import kotlin.math.PI
import kotlin.math.atan2
import kotlin.math.max
import kotlin.math.min

/**
 * Stroked L-brackets for the pairing scan frame — port of iOS `amneziaScanBracketStrokePath`
 * (`iosPairingQrOverlayWindow.mm`).
 * corner: 0=TL, 1=TR, 2=BL, 3=BR.
 */
object PairingQrScanBracketPaths {

    private fun Path.addCornerMinorArc(
        cx: Float,
        cy: Float,
        r: Float,
        sx: Float,
        sy: Float,
        ex: Float,
        ey: Float
    ) {
        var asRad = atan2((sy - cy).toDouble(), (sx - cx).toDouble())
        var aeRad = atan2((ey - cy).toDouble(), (ex - cx).toDouble())
        while (aeRad - asRad > PI) {
            aeRad -= 2.0 * PI
        }
        while (aeRad - asRad < -PI) {
            aeRad += 2.0 * PI
        }
        val minor = aeRad - asRad
        val startDeg = Math.toDegrees(asRad).toFloat()
        val sweepDeg = Math.toDegrees(minor).toFloat()
        addArc(RectF(cx - r, cy - r, cx + r, cy + r), startDeg, sweepDeg)
    }

    fun bracketStrokePath(corner: Int, x0: Float, y0: Float, s: Float, R: Float, L: Float, t: Float): Path {
        val r = max(1.5f, R - t * 0.5f)
        val p = Path()
        val yy = y0 + t * 0.5f
        val yyb = y0 + s - t * 0.5f
        val xx = x0 + t * 0.5f
        val xxb = x0 + s - t * 0.5f

        when (corner) {
            0 -> {
                val cTLx = x0 + R
                val cTLy = y0 + R
                val sTLx = x0 + R
                val sTLy = yy
                val eTLx = xx
                val eTLy = y0 + R
                p.moveTo(x0 + R + L, yy)
                p.lineTo(sTLx, sTLy)
                p.addCornerMinorArc(cTLx, cTLy, r, sTLx, sTLy, eTLx, eTLy)
                val yEndTL = min(y0 + R + L, y0 + s - R - t * 0.5f)
                p.lineTo(xx, max(yEndTL, y0 + R + 2f))
            }
            1 -> {
                val cTRx = x0 + s - R
                val cTRy = y0 + R
                val sTRx = x0 + s - R
                val sTRy = yy
                val eTRx = xxb
                val eTRy = y0 + R
                p.moveTo(x0 + s - R - L, yy)
                p.lineTo(sTRx, sTRy)
                p.addCornerMinorArc(cTRx, cTRy, r, sTRx, sTRy, eTRx, eTRy)
                val yEndTR = min(y0 + R + L, y0 + s - R - t * 0.5f)
                p.lineTo(xxb, max(yEndTR, y0 + R + 2f))
            }
            2 -> {
                val cBLx = x0 + R
                val cBLy = y0 + s - R
                val sBLx = x0 + R
                val sBLy = yyb
                val eBLx = xx
                val eBLy = y0 + s - R
                p.moveTo(x0 + R + L, yyb)
                p.lineTo(sBLx, sBLy)
                p.addCornerMinorArc(cBLx, cBLy, r, sBLx, sBLy, eBLx, eBLy)
                val yEndTopRef = max(min(y0 + R + L, y0 + s - R - t * 0.5f), y0 + R + 2f)
                val yLegBL = y0 + s + y0 - yEndTopRef
                p.lineTo(xx, yLegBL)
            }
            3 -> {
                val cBRx = x0 + s - R
                val cBRy = y0 + s - R
                val sBRx = x0 + s - R
                val sBRy = yyb
                val eBRx = xxb
                val eBRy = y0 + s - R
                p.moveTo(x0 + s - R - L, yyb)
                p.lineTo(sBRx, sBRy)
                p.addCornerMinorArc(cBRx, cBRy, r, sBRx, sBRy, eBRx, eBRy)
                val yEndTopRef = max(min(y0 + R + L, y0 + s - R - t * 0.5f), y0 + R + 2f)
                val yLegBR = y0 + s + y0 - yEndTopRef
                p.lineTo(xxb, yLegBR)
            }
        }
        return p
    }
}
