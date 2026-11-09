import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

AbstractButton {
    id: root

    property string subtitle
    property string footnote
    property string badgeText
    property string errorText
    property bool muted: false

    readonly property real badgeOffset: root.badgeText !== "" ? 16 : 0

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: implicitContentHeight + topPadding + bottomPadding

    topInset: root.badgeOffset
    bottomInset: errorLabel.visible ? errorLabel.implicitHeight + 4 : 0

    leftPadding: 16
    rightPadding: 16
    topPadding: root.badgeOffset + 12
    bottomPadding: bottomInset + 12

    spacing: 12

    hoverEnabled: true
    focusPolicy: Qt.NoFocus
    opacity: enabled ? 1.0 : 0.4

    icon.width: 24
    icon.height: 24
    icon.color: root.muted ? AmneziaStyle.color.iconSecondary : AmneziaStyle.color.iconDefault

    Accessible.role: Accessible.Button
    Accessible.name: root.text
    Accessible.description: root.subtitle

    background: Item {
        implicitWidth: 358

        Rectangle {
            anchors.fill: parent

            radius: 16
            color: root.muted ? AmneziaStyle.color.bgSurfaceAlt : AmneziaStyle.color.bgSurface
            opacity: root.muted ? 0.75 : 1.0
        }

        Rectangle {
            anchors.fill: parent

            radius: 16
            color: !root.muted && root.enabled && (root.hovered || root.down) ? (root.down ? AmneziaStyle.color.bgStateLayerPressed : AmneziaStyle.color.bgStateLayer)
                                                                             : AmneziaStyle.color.transparent

            Behavior on color {
                ColorAnimation { duration: 150 }
            }
        }

        RecommendationBadgeType {
            x: 16
            y: -root.badgeOffset

            visible: root.badgeText !== ""
            text: root.badgeText
        }

        SmallTextType {
            id: errorLabel

            anchors.top: parent.bottom
            anchors.topMargin: 4
            anchors.left: parent.left
            anchors.right: parent.right

            visible: root.errorText !== ""
            text: root.errorText
            color: AmneziaStyle.color.statusDestructive
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
            enabled: root.enabled
        }
    }

    contentItem: RowLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: root.subtitle !== "" || root.footnote !== "" ? Qt.AlignTop : Qt.AlignVCenter

            spacing: root.spacing

            IconImage {
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: root.icon.width
                Layout.preferredHeight: root.icon.height

                visible: root.icon.source.toString() !== ""
                source: root.icon.source
                sourceSize: Qt.size(root.icon.width, root.icon.height)
                color: root.icon.color
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop

                spacing: 4

                H3TextType {
                    Layout.fillWidth: true

                    visible: text !== ""
                    text: root.text
                    color: root.muted ? AmneziaStyle.color.textTertiary : AmneziaStyle.color.textPrimary
                }

                SmallTextType {
                    Layout.fillWidth: true

                    visible: text !== "" && !root.muted
                    text: root.subtitle
                    textFormat: Text.RichText

                    onLinkActivated: function(link) {
                        Qt.openUrlExternally(link)
                    }
                }

                CaptionTextType {
                    Layout.fillWidth: true

                    visible: text !== "" && root.muted
                    text: root.subtitle
                    textFormat: Text.RichText
                    color: AmneziaStyle.color.textQuaternary
                }

                SmallTextType {
                    Layout.fillWidth: true
                    Layout.topMargin: 8

                    visible: text !== ""
                    text: root.footnote
                    color: root.muted ? AmneziaStyle.color.textQuaternary : AmneziaStyle.color.textTertiary
                }
            }
        }

        IconButtonType {
            Layout.alignment: Qt.AlignTop

            icon.source: "qrc:/images/controls/chevron-right.svg"
            icon.color: AmneziaStyle.color.iconDefault
            opacity: root.muted ? 0.5 : 1.0
            enabled: root.enabled
            Accessible.name: root.text

            onClicked: root.clicked()
        }
    }
}
