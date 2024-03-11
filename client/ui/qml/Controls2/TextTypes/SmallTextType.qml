import QtQuick

Text {
    lineHeight: LanguageModel.getCurrentLanguageIndex() === 5 ? 26 : 20 // Burmese
    lineHeightMode: Text.FixedHeight

    color: "#D7D8DB"
    font.pixelSize: 14
    font.weight: 400
    font.family: "PT Root UI VF"

    wrapMode: Text.WordWrap
}
