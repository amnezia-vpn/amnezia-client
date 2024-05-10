import QtQuick

Text {
    lineHeight: 21.6 + LanguageModel.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: "#D7D8DB"
    font.pixelSize: 18
    font.weight: 400
    font.family: "Noto Sans"

    wrapMode: Text.Wrap
}
