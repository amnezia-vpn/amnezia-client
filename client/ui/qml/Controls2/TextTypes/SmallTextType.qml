import QtQuick

Text {
    lineHeight: 20 + LanguageModel.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: "#D7D8DB"
    font.pixelSize: 14
    font.weight: 400
    font.family: "Noto Sans"

    wrapMode: Text.WordWrap
}
