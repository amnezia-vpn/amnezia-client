import QtQuick

Text {
    lineHeight: LanguageModel.getCurrentLanguageIndex() === 5 ? 20 : 16 // Burmese
    lineHeightMode: Text.FixedHeight

    color: "#878B91"
    font.pixelSize: 13
    font.weight: 400
    font.family: "PT Root UI VF"
    font.letterSpacing: 0.02

    wrapMode: Text.WordWrap
}
