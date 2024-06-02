import QtQuick

Text {
    lineHeight: 38 + LanguageModel.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: "#D7D8DB"
    font.pixelSize: 32
    font.weight: 700
    font.family: "PT Root UI VF"
    font.letterSpacing: -1.0

    wrapMode: Text.WordWrap
}

