import QtQuick

Text {
    lineHeight: LanguageModel.getCurrentLanguageIndex() === 5 ? 50 : 38 // Burmese
    lineHeightMode: Text.FixedHeight

    color: "#D7D8DB"
    font.pixelSize: 36
    font.weight: 700
    font.family: "PT Root UI VF"
    font.letterSpacing: -1.08

    wrapMode: Text.WordWrap
}

