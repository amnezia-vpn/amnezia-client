pragma Singleton

import QtQuick

QtObject {
    property QtObject color: QtObject {
        readonly property color transparent: 'transparent'
        readonly property color paleGray: '#F4F4F5'
        readonly property color lightGray: '#D4D4D8'
        readonly property color mutedGray: '#A1A1AA'
        readonly property color charcoalGray: '#52525B'
        readonly property color slateGray: '#27272A'
        readonly property color onyxBlack: '#121212'
        readonly property color midnightBlack: '#0A0A0A'
        readonly property color goldenApricot: '#EAB308'
        readonly property color burntOrange: '#CA8A04'
        readonly property color mutedBrown: '#A16207'
        readonly property color richBrown: '#854D0E'
        readonly property color deepBrown: '#713F12'
        readonly property color vibrantRed: '#EB5757'
        readonly property color darkCharcoal: '#261E1A'
        readonly property color sheerWhite: Qt.rgba(1, 1, 1, 0.14)
        readonly property color translucentWhite: Qt.rgba(1, 1, 1, 0.09)
        readonly property color barelyTranslucentWhite: Qt.rgba(1, 1, 1, 0.06)
        readonly property color translucentMidnightBlack: Qt.rgba(10/255, 10/255, 10/255, 0.82)
        readonly property color softGoldenApricot: Qt.rgba(234/255, 179/255, 8/255, 0.28)
        readonly property color mistyGray: Qt.rgba(244/255, 244/255, 245/255, 0.82)
        readonly property color cloudyGray: Qt.rgba(244/255, 244/255, 245/255, 0.62)
        readonly property color pearlGray: '#FAFAFA'
        readonly property color translucentRichBrown: Qt.rgba(133/255, 77/255, 14/255, 0.24)
        readonly property color translucentSlateGray: Qt.rgba(82/255, 82/255, 91/255, 0.18)
        readonly property color translucentOnyxBlack: Qt.rgba(18/255, 18/255, 18/255, 0.18)
    }
}
