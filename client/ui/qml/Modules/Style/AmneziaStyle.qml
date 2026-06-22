pragma Singleton

import QtQuick

QtObject {
    id: root

    // Whether the dark color set is currently active. Driven from C++ via
    // AppearanceController (see main2.qml); defaults to dark to match the
    // historical look on first load and for any context without the binding.
    property bool isDark: true

    // Active palette. All call sites read AmneziaStyle.color.<name>; switching
    // this object re-evaluates every binding, so the whole UI re-themes live.
    readonly property QtObject color: isDark ? darkColor : lightColor

    // Color names are historical/literal (e.g. "midnightBlack"). To avoid
    // touching the hundreds of existing call sites, the light set keeps the same
    // names but assigns each its light-mode role equivalent (so "midnightBlack",
    // the app background, becomes white in light mode, and so on).

    readonly property QtObject darkColor: QtObject {
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

        // Checked switch (SwitcherType): brown track with an apricot knob in dark mode.
        // (Light mode flips to an apricot track with a white knob — see lightColor.)
        readonly property color switchCheckedTrack: richBrown
        readonly property color switchCheckedTrackBorder: richBrown
        readonly property color switchCheckedKnob: goldenApricot

        readonly property color primaryButton: paleGray
        readonly property color primaryButtonHovered: lightGray
        readonly property color primaryButtonPressed: mutedGray
        readonly property color primaryButtonDisabled: charcoalGray
        readonly property color primaryButtonText: midnightBlack

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

        readonly property string goldenApricotString: '#FBB26A'
    }

    readonly property QtObject lightColor: QtObject {
        readonly property color transparent: 'transparent'
        // Neutrals: in dark mode these run light-foreground -> deep-background.
        // In light mode they invert to dark-foreground -> light-background.
        readonly property color paleGray: '#16171A'          // primary text/icon
        readonly property color lightGray: '#2C2D30'          // bright secondary text
        readonly property color mutedGray: '#6D7177'          // muted secondary text
        readonly property color charcoalGray: '#C1C2C5'       // borders / disabled
        readonly property color slateGray: '#E3E4E7'          // hovered/elevated surface
        readonly property color onyxBlack: '#F2F3F5'          // card / surface background
        readonly property color midnightBlack: '#FFFFFF'      // app background
        readonly property color goldenApricot: goldenApricotString
        readonly property color benefitsPanelBackground: '#F2F3F5'
        // Brand accents are kept identical across themes.
        readonly property color softViolet: '#A87BE2'
        readonly property color burntOrange: '#A85809'
        readonly property color mutedBrown: '#84603D'
        readonly property color richBrown: '#633303'
        readonly property color deepBrown: '#402102'
        readonly property color vibrantRed: '#EB5757'
        readonly property color darkCharcoal: '#E3E4E7'       // subtle "in progress" ring
        readonly property color pearlGray: '#2C2D30'          // near-foreground text

        // "On" switch reads as a solid apricot track (the knob's brand color) with a
        // white knob — the conventional light-mode toggle look.
        readonly property color switchCheckedTrack: goldenApricot
        readonly property color switchCheckedTrackBorder: goldenApricot
        readonly property color switchCheckedKnob: '#FFFFFF'

        // Primary filled button: kept the softer apricot (the deeper #E38E41 is reserved
        // for highlights — outlines/text/icons — where contrast matters more).
        readonly property color primaryButton: '#FBB26A'
        readonly property color primaryButtonHovered: '#FFC078'
        readonly property color primaryButtonPressed: '#F2A04E'
        readonly property color primaryButtonDisabled: slateGray
        readonly property color primaryButtonText: '#1C1D21'

        // White overlays (hover/press tints) become black overlays on light.
        readonly property color sheerWhite: Qt.rgba(0, 0, 0, 0.10)
        readonly property color translucentWhite: Qt.rgba(0, 0, 0, 0.06)
        readonly property color barelyTranslucentWhite: Qt.rgba(0, 0, 0, 0.04)
        // Modal scrim stays a dark dim in both themes, just gentler on light.
        readonly property color translucentMidnightBlack: Qt.rgba(14/255, 14/255, 17/255, 0.4)
        readonly property color softGoldenApricot: Qt.rgba(251/255, 178/255, 106/255, 0.3)
        // Translucent light-gray foregrounds invert to translucent dark.
        readonly property color mistyGray: Qt.rgba(28/255, 29/255, 33/255, 0.8)
        readonly property color cloudyGray: Qt.rgba(28/255, 29/255, 33/255, 0.65)
        readonly property color translucentRichBrown: Qt.rgba(99/255, 51/255, 3/255, 0.26)
        readonly property color translucentSlateGray: Qt.rgba(44/255, 45/255, 48/255, 0.08)
        readonly property color translucentOnyxBlack: Qt.rgba(28/255, 29/255, 33/255, 0.06)

        // Deeper apricot than dark mode's #FBB26A — the pale tone is too low-contrast on white.
        readonly property string goldenApricotString: '#E38E41'
    }
}
