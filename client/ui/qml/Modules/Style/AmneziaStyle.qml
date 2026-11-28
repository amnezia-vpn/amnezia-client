pragma Singleton

import QtQuick

QtObject {
    id: root

    property int requestedColorScheme: Qt.ColorScheme.Unknown
    readonly property bool isLight: (requestedColorScheme !== Qt.ColorScheme.Unknown ? requestedColorScheme
                                                                                     : Application.styleHints.colorScheme) === Qt.ColorScheme.Light

    property int requestedAccent: 0
    readonly property var accents: [
        {
            swatch: '#FDBA74',
            light: { light: '#FDBA74', base: '#EA580C', dark: '#C2410C', deep: '#FDBA74', text: '#C2410C', border: '#EA580C',
                     stateLayer: '#1AF59E0B', subtle: '#33F59E0B', glow: '#FBB751' },
            dark: { light: '#FDBA74', base: '#F59E0B', dark: '#D97706', deep: '#78350F', text: '#FDBA74', border: '#FDBA74',
                    stateLayer: '#14FDBA74', subtle: '#4DD97706', glow: '#FBB751' }
        },
        {
            swatch: '#6E5EA6',
            light: { light: '#6E5EA6', base: '#563FA6', dark: '#453285', deep: '#CFC8EA', text: '#392A6F', border: '#563FA6',
                     stateLayer: '#1A563FA6', subtle: '#33563FA6', glow: '#6E5EA6' },
            dark: { light: '#6E5EA6', base: '#563FA6', dark: '#453285', deep: '#261C4A', text: '#6E5EA6', border: '#6E5EA6',
                    stateLayer: '#146E5EA6', subtle: '#4D453285', glow: '#6E5EA6' }
        },
        {
            swatch: '#80C9C1',
            light: { light: '#80C9C1', base: '#3FA69B', dark: '#32857C', deep: '#C8EAE6', text: '#2A6F67', border: '#3FA69B',
                     stateLayer: '#1A3FA69B', subtle: '#333FA69B', glow: '#80C9C1' },
            dark: { light: '#80C9C1', base: '#3FA69B', dark: '#32857C', deep: '#1C4A45', text: '#80C9C1', border: '#80C9C1',
                    stateLayer: '#1480C9C1', subtle: '#4D32857C', glow: '#80C9C1' }
        },
        {
            swatch: '#95C7DB',
            light: { light: '#95C7DB', base: '#3A8BAB', dark: '#2F6F89', deep: '#C6E1EC', text: '#275D72', border: '#3A8BAB',
                     stateLayer: '#1A3A8BAB', subtle: '#333A8BAB', glow: '#95C7DB' },
            dark: { light: '#95C7DB', base: '#3A8BAB', dark: '#2F6F89', deep: '#1A3E4C', text: '#95C7DB', border: '#95C7DB',
                    stateLayer: '#1495C7DB', subtle: '#4D2F6F89', glow: '#95C7DB' }
        }
    ]
    readonly property var accent: {
        const palette = accents[requestedAccent] ?? accents[0]
        return isLight ? palette.light : palette.dark
    }

    property QtObject color: QtObject {
        readonly property color transparent: 'transparent'

        readonly property color textPrimary: root.isLight ? '#18181B' : '#FAFAFA'
        readonly property color textSecondary: root.isLight ? '#3F3F46' : '#E4E4E7'
        readonly property color textTertiary: root.isLight ? '#71717A' : '#A1A1AA'
        readonly property color textQuaternary: root.isLight ? '#909099' : '#86868F'
        readonly property color textInverted: root.isLight ? '#FAFAFA' : '#09090B'
        readonly property color textAccent: root.accent.text
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
        readonly property color bgStateLayerAccent: root.accent.stateLayer
        readonly property color bgSurfaceInverse: root.isLight ? '#27272A' : '#E4E4E7'
        readonly property color bgSurfaceInverseAlt: root.isLight ? '#3F3F46' : '#D4D4D8'
        readonly property color bgControlTrack: root.isLight ? '#E4E4E7' : '#1F1F23'
        readonly property color bgField: root.isLight ? '#FFFFFF' : '#1F1F23'
        readonly property color bgSelected: root.isLight ? '#EDEDF0' : '#1F1F23'
        readonly property color bgAccentSubtle: root.accent.subtle
        readonly property color bgStaticWhite: '#FFFFFF'

        readonly property color bgAction: root.isLight ? '#18181B' : '#E4E4E7'
        readonly property color bgActionHover: root.isLight ? '#27272A' : '#D4D4D8'
        readonly property color bgActionPress: root.isLight ? '#3F3F46' : '#A1A1AA'

        readonly property color borderFaint: root.isLight ? '#E4E4E7' : '#313136'
        readonly property color borderSoft: root.isLight ? '#D4D4D8' : '#3F3F46'
        readonly property color borderMedium: root.isLight ? '#A1A1AA' : '#52525B'
        readonly property color borderBold: root.isLight ? '#71717A' : '#A1A1AA'
        readonly property color borderEmphasis: root.isLight ? '#27272A' : '#E4E4E7'
        readonly property color borderAccent: root.accent.border
        readonly property color borderField: root.isLight ? '#D4D4D8' : '#52525B'
        readonly property color borderFocus: root.isLight ? '#27272A' : '#E4E4E7'

        readonly property color accentLight: root.accent.light
        readonly property color accentDefault: root.accent.base
        readonly property color accentDark: root.accent.dark
        readonly property color accentDeep: root.accent.deep

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
        readonly property color shadowGlow: root.accent.glow
    }
}
