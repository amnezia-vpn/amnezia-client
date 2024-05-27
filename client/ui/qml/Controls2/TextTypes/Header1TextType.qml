import QtQuick

import Style 1.0

Text {
    lineHeight: 38 + LanguageModel.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: AmneziaStyle.color.white
    font.pixelSize: 36
    font.weight: 700
    font.family: "PT Root UI VF"
    font.letterSpacing: -1.08

    wrapMode: Text.WordWrap
}

