import QtQuick

import Style 1.0

Text {
    id: root

    readonly property real halfLeading: (root.lineHeight - fontMetrics.height) / 2
    readonly property real overflow: Math.max(0, Math.ceil(fontMetrics.height) - root.lineHeight)

    lineHeight: 14 + LanguageUiController.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    topPadding: root.halfLeading
    bottomPadding: -root.halfLeading - root.overflow

    color: AmneziaStyle.color.textTertiary
    font.pixelSize: 12
    font.weight: 400
    font.family: "PT Root UI VF"
    font.letterSpacing: 0
    font.features: { "ss08": 1, "ss09": 1, "lnum": 1, "tnum": 1 }

    wrapMode: Text.WordWrap

    FontMetrics {
        id: fontMetrics
        font: root.font
    }
}
