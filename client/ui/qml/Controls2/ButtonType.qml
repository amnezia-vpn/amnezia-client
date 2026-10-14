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

    property string leftImageSource
    property string rightImageSource

    implicitHeight: d.height

    horizontalPadding: 12
    verticalPadding: 8

    hoverEnabled: true

    opacity: root.enabled ? 1 : d.disabledOpacity

    QtObject {
        id: d

        readonly property bool small: root.size === ButtonType.Small
        readonly property bool outlined: root.variant === ButtonType.Secondary || root.variant === ButtonType.Destructive
        readonly property bool active: root.down || root.hovered

        readonly property int height: root.size === ButtonType.Large ? 48 : (root.size === ButtonType.Medium ? 40 : 36)
        readonly property int radius: root.size === ButtonType.Large ? 16 : (root.size === ButtonType.Medium ? 14 : 12)
        readonly property int iconSize: small ? 20 : 24
        readonly property int gap: root.size === ButtonType.Large ? 4 : 2

        readonly property real disabledOpacity: {
            switch (root.variant) {
            case ButtonType.Primary: return 0.25
            case ButtonType.Destructive: return 0.36
            case ButtonType.Text: return 0.5
            case ButtonType.Surface: return 0.5
            default: return 1
            }
        }

        readonly property color backgroundColor: {
            switch (root.variant) {
            case ButtonType.Primary:
                return root.down ? AmneziaStyle.color.buttonPrimaryPress
                                 : (root.hovered ? AmneziaStyle.color.buttonPrimaryHover : AmneziaStyle.color.buttonPrimaryBg)
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
                return active ? AmneziaStyle.color.borderBold : AmneziaStyle.color.borderSoft
            }
            if (root.variant === ButtonType.Text && root.down) {
                return AmneziaStyle.color.borderEmphasis
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
                return root.enabled ? AmneziaStyle.color.textSecondary : AmneziaStyle.color.textQuaternary
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
                return root.enabled ? AmneziaStyle.color.iconDefault : AmneziaStyle.color.iconTertiary
            default: return AmneziaStyle.color.iconDefault
            }
        }
    }

    background: Rectangle {
        implicitHeight: d.height

        radius: d.radius
        color: d.backgroundColor
        border.width: d.borderColor.a > 0 ? 1 : 0
        border.color: d.borderColor

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        Rectangle {
            anchors.fill: parent

            radius: parent.radius
            color: AmneziaStyle.color.bgStateLayer
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

            IconImage {
                Layout.preferredWidth: d.iconSize
                Layout.preferredHeight: d.iconSize

                visible: root.leftImageSource !== ""
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

                wrapMode: Text.NoWrap
                elide: Text.ElideRight
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
