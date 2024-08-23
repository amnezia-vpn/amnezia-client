import QtQuick

import Style 1.0

Text {
    lineHeight: 21.6 + LanguageModel.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: AmneziaStyle.color.paleGray
    font.pixelSize: 18
    font.weight: 400
    font.family: "PT Root UI VF"

    wrapMode: Text.Wrap
}
