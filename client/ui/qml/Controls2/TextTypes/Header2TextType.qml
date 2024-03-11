import QtQuick

Text {
    lineHeight: LanguageModel.getCurrentLanguageIndex() === 5 ? 40 : 30 // Burmese
    lineHeightMode: Text.FixedHeight

    color: "#D7D8DB"
    font.pixelSize: 25
    font.weight: 700
    font.family: "PT Root UI VF"

    wrapMode: Text.WordWrap
}
