import QtQuick

import Style 1.0

Text {
    lineHeight: 30 + LanguageModel.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: AmneziaStyle.color.paleGray
    font.pixelSize: 25
    font.weight: 700
    font.family: "PT Root UI VF"

    wrapMode: Text.WordWrap
}
