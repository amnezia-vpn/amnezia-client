import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Button {
    id: root

    enum Variant { Primary, Secondary, Destructive, Text, Tonal, Surface }
    enum Size { Large, Medium, Small }

    property int variant: ButtonType.Primary
    property int size: ButtonType.Large
    property bool destructive: false
    property bool loading: false

    property string leftImageSource
    property string rightImageSource

    implicitHeight: Math.max(d.height, implicitContentHeight + topPadding + bottomPadding)

    horizontalPadding: 12
    verticalPadding: 8

    hoverEnabled: true

    opacity: root.enabled || root.loading ? 1 : 0.4

    QtObject {
        id: d

        readonly property bool small: root.size === ButtonType.Small
        readonly property bool textVariant: root.variant === ButtonType.Text
        readonly property bool outlined: root.variant === ButtonType.Secondary || root.variant === ButtonType.Destructive
        readonly property bool active: root.down || root.hovered
        readonly property bool pressedRing: root.down && root.variant === ButtonType.Primary && root.size === ButtonType.Large

        readonly property int height: root.size === ButtonType.Large ? 48 : (root.size === ButtonType.Medium ? 40 : 36)
        readonly property int radius: small && !textVariant ? 12 : (root.size === ButtonType.Medium && !textVariant ? 14 : 16)
        readonly property int iconSize: small || (textVariant && root.size === ButtonType.Medium) ? 20 : 24
        readonly property int gap: root.size === ButtonType.Large || textVariant ? 4 : 2

        readonly property color backgroundColor: {
            switch (root.variant) {
            case ButtonType.Primary:
                return root.down ? AmneziaStyle.color.bgActionPress
                                 : (root.hovered ? AmneziaStyle.color.bgActionHover : AmneziaStyle.color.bgAction)
            case ButtonType.Secondary:
            case ButtonType.Destructive:
            case ButtonType.Surface:
                return root.down ? AmneziaStyle.color.bgPanel
                                 : (root.hovered ? AmneziaStyle.color.bgSurfaceAlt : AmneziaStyle.color.bgSurface)
            case ButtonType.Tonal:
                return AmneziaStyle.color.bgSurfaceAlt
            default:
                return AmneziaStyle.color.transparent
            }
        }

        readonly property bool stateLayerVisible: active && (root.variant === ButtonType.Text || root.variant === ButtonType.Tonal)

        readonly property color borderColor: {
            if (outlined) {
                return active || root.loading ? AmneziaStyle.color.borderBold : AmneziaStyle.color.borderSoft
            }
            return AmneziaStyle.color.transparent
        }

        readonly property color labelColor: {
            switch (root.variant) {
            case ButtonType.Primary: return AmneziaStyle.color.textInverted
            case ButtonType.Destructive: return AmneziaStyle.color.statusDestructive
            case ButtonType.Text: return root.destructive ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.textAccent
            case ButtonType.Secondary:
            case ButtonType.Tonal:
                return AmneziaStyle.color.textSecondary
            default: return AmneziaStyle.color.textSecondary
            }
        }

        readonly property color iconColor: {
            switch (root.variant) {
            case ButtonType.Primary: return AmneziaStyle.color.iconInverted
            case ButtonType.Destructive: return AmneziaStyle.color.statusDestructive
            case ButtonType.Text: return root.destructive ? AmneziaStyle.color.statusDestructive : AmneziaStyle.color.textAccent
            case ButtonType.Secondary:
            case ButtonType.Tonal:
                return AmneziaStyle.color.iconDefault
            default: return AmneziaStyle.color.iconDefault
            }
        }
    }

    background: Rectangle {
        implicitHeight: Math.max(d.height, implicitContentHeight + topPadding + bottomPadding)

        radius: d.pressedRing ? d.radius + 2 : d.radius
        color: d.pressedRing ? AmneziaStyle.color.transparent : d.backgroundColor
        border.width: d.pressedRing || d.borderColor.a > 0 ? 1 : 0
        border.color: d.pressedRing ? AmneziaStyle.color.borderEmphasis : d.borderColor

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: 2

            visible: d.pressedRing
            radius: d.radius
            color: d.backgroundColor
        }

        Rectangle {
            anchors.fill: parent

            radius: parent.radius
            color: root.down ? AmneziaStyle.color.bgStateLayerPressed : AmneziaStyle.color.bgStateLayer
            visible: d.stateLayerVisible
        }

        FocusRingType {
            radius: d.radius
            visible: root.visualFocus
        }
    }

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        RowLayout {
            id: row

            anchors.centerIn: parent
            width: Math.min(implicitWidth, parent.width)

            spacing: d.gap

            LoaderRingType {
                Layout.preferredWidth: d.iconSize
                Layout.preferredHeight: d.iconSize

                visible: root.loading
                size: d.iconSize
                arcColor: root.variant === ButtonType.Primary ? AmneziaStyle.color.iconInverted : AmneziaStyle.color.textPrimary
                trackColor: root.variant === ButtonType.Primary ? Qt.alpha(AmneziaStyle.color.iconInverted, 0.3) : AmneziaStyle.color.textTertiary
            }

            IconImage {
                Layout.preferredWidth: d.iconSize
                Layout.preferredHeight: d.iconSize

                visible: root.leftImageSource !== "" && !root.loading
                source: root.leftImageSource
                sourceSize: Qt.size(d.iconSize, d.iconSize)
                color: d.iconColor
            }

            ButtonTextType {
                Layout.fillWidth: true

                visible: text !== ""
                text: root.text

                leftPadding: 2
                rightPadding: 2

                color: d.labelColor
                lineHeight: (d.small ? 18 : 24) + LanguageUiController.getLineHeightAppend()
                font.pixelSize: d.small ? 14 : 16
                font.weight: root.variant === ButtonType.Text ? 400 : 600

                wrapMode: Text.Wrap
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            IconImage {
                Layout.preferredWidth: d.iconSize
                Layout.preferredHeight: d.iconSize

                visible: root.rightImageSource !== ""
                source: root.rightImageSource
                sourceSize: Qt.size(d.iconSize, d.iconSize)
                color: d.iconColor
            }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
