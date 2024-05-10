import QtQuick

Text {
    lineHeight: 24 + LanguageModel.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: "#D7D8DB"
    font.pixelSize: 16
    font.weight: 400
    font.family: "Noto Sans"

    wrapMode: Text.WordWrap
}
