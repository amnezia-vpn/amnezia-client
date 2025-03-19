pragma Singleton

import QtQuick

QtObject {
    property QtObject color: QtObject {
        readonly property color transparent: 'transparent'
        readonly property color gray1: '#F2F2F7'
        readonly property color gray2: '#E5E5EA'
        readonly property color gray3: '#D1D1D6'
        readonly property color gray4: '#C7C7CC'
        readonly property color gray5: '#AEAEB2'
        readonly property color gray6: '#8E8E93'
        readonly property color gray7: '#7C7C83'
        readonly property color gray8: '#707075'
        readonly property color gray9: '#57575B'
        readonly property color accent1: '#007AFF'
        readonly property color accent2: '#0B6EDA'
        readonly property color accent3: '#1256A1'
        readonly property color error: '#FF3B30'
        readonly property color warning: '#FF9500'
        readonly property color success: '#34C759'
        readonly property color black: '#000000'
        readonly property color white: '#FFFFFF'

        readonly property color transparentBlack: Qt.rgba(14/255, 14/255, 17/255, 0.8)
    }

    readonly property string font: "Vela Sans GX"
}
