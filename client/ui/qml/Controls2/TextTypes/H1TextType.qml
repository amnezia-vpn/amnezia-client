import QtQuick

import Style 1.0

Text {
    id: root

    readonly property real halfLeading: Math.max(0, (root.lineHeight - fontMetrics.height) / 2)

    lineHeight: 44 + LanguageUiController.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    topPadding: root.halfLeading
    bottomPadding: -root.halfLeading

    color: AmneziaStyle.color.textPrimary
    font.pixelSize: 30
    font.weight: 700
    font.family: "PT Root UI VF"
    font.letterSpacing: 0.4
    font.features: { "ss08": 1, "ss09": 1, "lnum": 1, "tnum": 1 }

    wrapMode: Text.WordWrap

    FontMetrics {
        id: fontMetrics
        font: root.font
    }
}
