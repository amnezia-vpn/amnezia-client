import QtQuick

import Style 1.0

Text {
    lineHeight: 10 + LanguageUiController.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: AmneziaStyle.color.midnightBlack
    font.pixelSize: 11
    font.weight: Font.Medium
    font.family: "PT Root UI VF"

    wrapMode: Text.NoWrap
}
