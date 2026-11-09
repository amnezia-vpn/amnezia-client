import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

ItemDelegate {
    id: root

    property string subtitle
    property bool critical: false

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: implicitContentHeight + topPadding + bottomPadding

    padding: 16
    spacing: 12

    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    opacity: enabled ? 1.0 : 0.4

    icon.width: 26
    icon.height: 26

    Accessible.name: root.text
    Accessible.description: root.subtitle

    Keys.onEnterPressed: root.click()
    Keys.onReturnPressed: root.click()

    background: Rectangle {
        implicitWidth: 358

        radius: 16
        color: {
            if (!root.enabled) {
                return AmneziaStyle.color.bgSurface
            }
            return root.critical ? AmneziaStyle.color.promoOverlay : AmneziaStyle.color.bgBase
        }
        border.width: 1
        border.color: {
            if (root.enabled && root.down) {
                return AmneziaStyle.color.borderMedium
            }
            return root.critical ? AmneziaStyle.color.promoRecommendation : AmneziaStyle.color.borderSoft
        }

        Rectangle {
            anchors.fill: parent

            radius: parent.radius
            color: root.down ? AmneziaStyle.color.bgStateLayerPressed : AmneziaStyle.color.bgStateLayer
            opacity: root.enabled && (root.down || root.hovered) ? 1 : 0

            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }

        FocusRingType {
            radius: 16
            visible: root.visualFocus
        }

        HoverHandler {
            cursorShape: Qt.PointingHandCursor
            enabled: root.enabled
        }
    }

    contentItem: RowLayout {
        spacing: root.spacing

        Image {
            Layout.preferredWidth: root.icon.width
            Layout.preferredHeight: root.icon.height

            visible: root.icon.source.toString() !== ""
            source: root.icon.source
            sourceSize: Qt.size(root.icon.width, root.icon.height)
        }

        ColumnLayout {
            Layout.fillWidth: true

            spacing: 4

            H3TextType {
                Layout.fillWidth: true

                text: root.text
                textFormat: Text.RichText
            }

            SmallTextType {
                Layout.fillWidth: true

                visible: text !== ""
                text: root.subtitle
            }
        }

        Item {
            Layout.preferredWidth: 36
            Layout.preferredHeight: 36
            Layout.leftMargin: 4 - root.spacing

            IconImage {
                anchors.centerIn: parent
                width: 20
                height: 20

                source: "qrc:/images/controls/chevron-right.svg"
                sourceSize: Qt.size(20, 20)
                color: AmneziaStyle.color.iconDefault
            }
        }
    }
}
