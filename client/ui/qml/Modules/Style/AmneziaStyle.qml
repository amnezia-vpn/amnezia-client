pragma Singleton

import QtQuick

QtObject {
    id: root

    property int requestedColorScheme: Qt.ColorScheme.Unknown
    readonly property bool isLight: (requestedColorScheme !== Qt.ColorScheme.Unknown ? requestedColorScheme
                                                                                     : Application.styleHints.colorScheme) === Qt.ColorScheme.Light

    property QtObject color: QtObject {
        readonly property color transparent: 'transparent'

        readonly property color textPrimary: root.isLight ? '#18181B' : '#FAFAFA'
        readonly property color textSecondary: root.isLight ? '#3F3F46' : '#E4E4E7'
        readonly property color textTertiary: root.isLight ? '#71717A' : '#A1A1AA'
        readonly property color textQuaternary: root.isLight ? '#909099' : '#86868F'
        readonly property color textInverted: root.isLight ? '#FAFAFA' : '#09090B'
        readonly property color textAccent: root.isLight ? '#C2410C' : '#FDBA74'
        readonly property color textInfoInverse: root.isLight ? '#7DD3FC' : '#0369A1'
        readonly property color textStaticWhite: '#FFFFFF'

        readonly property color bgBase: root.isLight ? '#F4F4F5' : '#101012'
        readonly property color bgSurface: root.isLight ? '#FFFFFF' : '#18181B'
        readonly property color bgSurfaceAlt: root.isLight ? '#EDEDF0' : '#1F1F23'
        readonly property color bgSheet: root.isLight ? '#FFFFFF' : '#313136'
        readonly property color bgPanel: root.isLight ? '#E4E4E7' : '#3F3F46'
        readonly property color bgPanelAlt: root.isLight ? '#D4D4D8' : '#52525B'
        readonly property color bgOverlay: root.isLight ? '#6609090B' : '#8009090B'
        readonly property color bgStateLayer: root.isLight ? '#0D09090B' : '#14FAFAFA'
        readonly property color bgStateLayerPressed: root.isLight ? '#1A09090B' : '#1AFAFAFA'
        readonly property color bgStateLayerAccent: root.isLight ? '#1AF59E0B' : '#14FDBA74'
        readonly property color bgSurfaceInverse: root.isLight ? '#27272A' : '#E4E4E7'
        readonly property color bgSurfaceInverseAlt: root.isLight ? '#3F3F46' : '#D4D4D8'
        readonly property color bgControlTrack: root.isLight ? '#E4E4E7' : '#1F1F23'
        readonly property color bgField: root.isLight ? '#FFFFFF' : '#1F1F23'
        readonly property color bgSelected: root.isLight ? '#EDEDF0' : '#1F1F23'
        readonly property color bgAccentSubtle: root.isLight ? '#33F59E0B' : '#4DD97706'
        readonly property color bgStaticWhite: '#FFFFFF'

        readonly property color bgAction: root.isLight ? '#18181B' : '#E4E4E7'
        readonly property color bgActionHover: root.isLight ? '#27272A' : '#D4D4D8'
        readonly property color bgActionPress: root.isLight ? '#3F3F46' : '#A1A1AA'

        readonly property color borderFaint: root.isLight ? '#E4E4E7' : '#313136'
        readonly property color borderSoft: root.isLight ? '#D4D4D8' : '#3F3F46'
        readonly property color borderMedium: root.isLight ? '#A1A1AA' : '#52525B'
        readonly property color borderBold: root.isLight ? '#71717A' : '#A1A1AA'
        readonly property color borderEmphasis: root.isLight ? '#27272A' : '#E4E4E7'
        readonly property color borderAccent: root.isLight ? '#EA580C' : '#FDBA74'
        readonly property color borderField: root.isLight ? '#D4D4D8' : '#52525B'
        readonly property color borderFocus: root.isLight ? '#27272A' : '#E4E4E7'

        readonly property color accentLight: '#FDBA74'
        readonly property color accentDefault: root.isLight ? '#EA580C' : '#F59E0B'
        readonly property color accentDark: root.isLight ? '#C2410C' : '#D97706'
        readonly property color accentDeep: root.isLight ? '#FDBA74' : '#78350F'

        readonly property color statusDestructive: root.isLight ? '#DC2626' : '#F87171'
        readonly property color statusWarning: root.isLight ? '#CA8A04' : '#EAB308'
        readonly property color statusSuccess: root.isLight ? '#16A34A' : '#4ADE80'

        readonly property color promoRecommendation: root.isLight ? '#7C3AED' : '#A78BFA'
        readonly property color promoUpdate: root.isLight ? '#0284C7' : '#38BDF8'
        readonly property color promoOverlay: '#148B5CF6'
        readonly property color promoPink: '#DB2777'
        readonly property color promoPinkDeep: '#9D174D'

        readonly property color iconDefault: root.isLight ? '#27272A' : '#E4E4E7'
        readonly property color iconSecondary: root.isLight ? '#71717A' : '#A1A1AA'
        readonly property color iconTertiary: root.isLight ? '#D4D4D8' : '#3F3F46'
        readonly property color iconInverted: root.isLight ? '#FAFAFA' : '#09090B'

        readonly property color shadowLayer1: root.isLight ? '#14000000' : '#26000000'
        readonly property color shadowLayer2: root.isLight ? '#26000000' : '#4D000000'
        readonly property color shadowGlow: '#FBB751'
    }
}
