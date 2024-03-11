import QtQuick

Text {
    lineHeight: LanguageModel.getCurrentLanguageIndex() === 5 ? 30 : 24 // Burmese
    lineHeightMode: Text.FixedHeight

    color: "#D7D8DB"
    font.pixelSize: 16
    font.weight: 400
    font.family: "PT Root UI VF"

    wrapMode: Text.WordWrap
}
