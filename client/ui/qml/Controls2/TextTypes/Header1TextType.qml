import QtQuick

Text {
    lineHeight: 38 + LanguageModel.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: "#D7D8DB"
    font.pixelSize: 36
    font.weight: 700
    font.family: "Noto Sans"
    font.letterSpacing: -1.08

    wrapMode: Text.WordWrap
}

