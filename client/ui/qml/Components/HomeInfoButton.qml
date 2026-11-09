import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

AbstractButton {
    id: root

    property string leftImageSource
    property color leftImageColor: AmneziaStyle.color.iconDefault
    property int leftImageWidth: 24
    property int leftImageHeight: 24
    property string rightImageSource
    property bool largeText: false
    property int maximumLineCount: 1

    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: Math.max(40, implicitContentHeight + topPadding + bottomPadding)

    horizontalPadding: 16
    verticalPadding: 8

    hoverEnabled: enabled
    focusPolicy: enabled ? Qt.TabFocus : Qt.NoFocus

    Accessible.name: root.text

    background: Rectangle {
        radius: 16
        color: root.enabled && (root.hovered || root.down) ? AmneziaStyle.color.bgStateLayer : AmneziaStyle.color.transparent
        border.width: root.enabled && root.down ? 1 : 0
        border.color: AmneziaStyle.color.borderEmphasis

        Behavior on color {
            ColorAnimation { duration: 150 }
        }

        FocusRingType {
            radius: 16
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

            spacing: 8

            Image {
                Layout.preferredWidth: root.leftImageWidth
                Layout.preferredHeight: root.leftImageHeight

                visible: root.leftImageSource !== "" && root.leftImageColor.a === 0
                source: visible ? root.leftImageSource : ""
                sourceSize: Qt.size(root.leftImageWidth, root.leftImageHeight)
                fillMode: Image.PreserveAspectFit
            }

            IconImage {
                Layout.preferredWidth: root.leftImageWidth
                Layout.preferredHeight: root.leftImageHeight

                visible: root.leftImageSource !== "" && root.leftImageColor.a > 0
                source: visible ? root.leftImageSource : ""
                sourceSize: Qt.size(root.leftImageWidth, root.leftImageHeight)
                color: root.leftImageColor
            }

            BodyTextType {
                Layout.fillWidth: true

                visible: root.largeText
                text: root.text
                color: AmneziaStyle.color.textTertiary
                maximumLineCount: root.maximumLineCount
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }

            SmallTextType {
                Layout.fillWidth: true

                visible: !root.largeText
                text: root.text
                maximumLineCount: root.maximumLineCount
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }

            IconImage {
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16

                visible: root.rightImageSource !== ""
                source: root.rightImageSource
                sourceSize: Qt.size(16, 16)
                color: AmneziaStyle.color.iconDefault
            }
        }
    }

    HoverHandler {
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
