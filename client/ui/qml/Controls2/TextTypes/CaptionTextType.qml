import QtQuick

Text {
    lineHeight: 16 + LanguageModel.getLineHeightAppend()
    lineHeightMode: Text.FixedHeight

    color: "#0E0E11"
    font.pixelSize: 13
    font.weight: 400
    font.family: "Noto Sans"
    font.letterSpacing: 0.02

    wrapMode: Text.Wrap
}
