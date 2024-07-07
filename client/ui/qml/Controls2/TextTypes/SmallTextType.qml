import QtQuick

import Style 1.0

Text {
    lineHeight: 20 + LanguageModel.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: AmneziaStyle.color.white
    font.pixelSize: 14
    font.weight: 400
    font.family: "PT Root UI VF"

    wrapMode: Text.WordWrap
}
