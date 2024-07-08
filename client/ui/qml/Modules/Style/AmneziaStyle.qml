pragma Singleton

import QtQuick

QtObject {
    property QtObject color: QtObject {
        readonly property color transparent: 'transparent'
        readonly property color white: '#D7D8DB'
        readonly property color whiteHovered: '#C1C2C5'
        readonly property color grey: '#878B91'
        readonly property color greyDisabled: '#494B50'
        readonly property color greyDark: '#2C2D30'
        readonly property color blackLight: '#1C1D21'
        readonly property color blackHovered: '#01010114'
        readonly property color blackPressed: '#0101011f'
        readonly property color black: '#0E0E11'
        readonly property color orange: '#FBB26A'
        readonly property color orangeDark: '#A85809'
        readonly property color brownLight: '#84603D'
        readonly property color brown: '#633303'
        readonly property color brownDark: '#402102'
        readonly property color red: '#EB5757'

        readonly property color connectionInProgress: '#261E1A'
    }
}