pragma Singleton

import QtQuick

QtObject {
    id: root

    property QtObject color: QtObject {
        readonly property color transparent: 'transparent'
        readonly property color paleGray: '#D7D8DB'
        readonly property color lightGray: '#C1C2C5'
        readonly property color mutedGray: '#878B91'
        readonly property color charcoalGray: '#494B50'
        readonly property color slateGray: '#2C2D30'
        readonly property color onyxBlack: '#1C1D21'
        readonly property color midnightBlack: '#0E0E11'
        readonly property color goldenApricot: goldenApricotString
        readonly property color benefitsPanelBackground: '#1C1C1E'
        readonly property color softViolet: '#A87BE2'
        readonly property color burntOrange: '#A85809'
        readonly property color mutedBrown: '#84603D'
        readonly property color richBrown: '#633303'
        readonly property color deepBrown: '#402102'
        readonly property color vibrantRed: '#EB5757'
        readonly property color darkCharcoal: '#261E1A'
        readonly property color pearlGray: '#EAEAEC'

        readonly property color sheerWhite: Qt.rgba(1, 1, 1, 0.12)
        readonly property color translucentWhite: Qt.rgba(1, 1, 1, 0.08)
        readonly property color barelyTranslucentWhite: Qt.rgba(1, 1, 1, 0.05)
        readonly property color translucentMidnightBlack: Qt.rgba(14/255, 14/255, 17/255, 0.8)
        readonly property color softGoldenApricot: Qt.rgba(251/255, 178/255, 106/255, 0.3)
        readonly property color mistyGray: Qt.rgba(215/255, 216/255, 219/255, 0.8)
        readonly property color cloudyGray: Qt.rgba(215/255, 216/255, 219/255, 0.65)
        readonly property color translucentRichBrown: Qt.rgba(99/255, 51/255, 3/255, 0.26)
        readonly property color translucentSlateGray: Qt.rgba(85/255, 86/255, 92/255, 0.13)
        readonly property color translucentOnyxBlack: Qt.rgba(28/255, 29/255, 33/255, 0.13)
        readonly property color faintGoldenApricot: Qt.rgba(251/255, 178/255, 106/255, 0.14)

        readonly property string goldenApricotString: '#FBB26A'
    }

    // Appearance of the keyboard/D-pad focus indicator. On a TV the control is
    // several metres away and a 1px hairline in the same tone as the control is
    // not readable, so there the indicator switches to a thick accent outline
    // with a tinted fill. Set from main2.qml.
    property QtObject focus: QtObject {
        property bool isOnTv: false

        readonly property color borderColor: isOnTv ? root.color.goldenApricot
                                                    : root.color.paleGray
        readonly property int borderWidth: isOnTv ? 3 : 1
        // off a TV the outline shares the control's own tone, so the control is
        // shrunk a little to leave a dark gap that makes the outline readable
        readonly property int backgroundInset: isOnTv ? 0 : 2
        // breathing room for controls whose content sits flush against their own
        // edge, so the outline does not run straight through the label
        readonly property int contentPadding: isOnTv ? 16 : 0
        // kept light: the fill sits over the control's own label
        readonly property color overlayColor: isOnTv ? root.color.faintGoldenApricot
                                                     : root.color.transparent
    }
}
