import QtQuick

import Style 1.0

Text {
    id: root

    readonly property real halfLeading: Math.max(0, (root.lineHeight - fontMetrics.height) / 2)
    readonly property real alignmentShift: {
        const extra = Math.ceil(fontMetrics.height) - root.lineHeight
        if (root.verticalAlignment === Text.AlignVCenter) {
            return extra / 2
        }
        return root.verticalAlignment === Text.AlignBottom ? extra : 0
    }

    lineHeight: 24 + LanguageUiController.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    topPadding: root.halfLeading + root.alignmentShift
    bottomPadding: -root.halfLeading - root.alignmentShift

    color: AmneziaStyle.color.textPrimary
    font.pixelSize: 18
    font.weight: 400
    font.family: "PT Root UI VF"
    font.letterSpacing: -0.4
    font.features: { "ss08": 1, "ss09": 1, "lnum": 1, "tnum": 1 }

    wrapMode: Text.WordWrap

    FontMetrics {
        id: fontMetrics
        font: root.font
    }
}
