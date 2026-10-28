import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

AbstractButton {
    id: root

    enum Size { Small, Medium }

    property int size: LinkButtonType.Small
    property bool colored: true
    property string leftImageSource
    property string rightImageSource

    padding: 0

    hoverEnabled: true

    opacity: root.enabled ? 1 : 0.4

    QtObject {
        id: d

        readonly property color contentColor: {
            if (!root.colored) {
                return AmneziaStyle.color.textTertiary
            }
            if (root.down) {
                return AmneziaStyle.color.accentDeep
            }
            return root.hovered ? AmneziaStyle.color.accentDefault : AmneziaStyle.color.textAccent
        }
    }

    background: Item {
        FocusRingType {
            radius: 4
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

            IconImage {
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14

                visible: root.leftImageSource !== ""
                source: root.leftImageSource
                sourceSize: Qt.size(14, 14)
                color: d.contentColor
            }

            SmallTextType {
                Layout.fillWidth: true

                text: root.text
                color: d.contentColor
                font.weight: 600
                font.letterSpacing: -0.4
                font.pixelSize: root.size === LinkButtonType.Medium ? 16 : 14
                lineHeight: (root.size === LinkButtonType.Medium ? 24 : 18) + LanguageUiController.getLineHeightAppend()

                wrapMode: Text.NoWrap
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }

            IconImage {
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14

                visible: root.rightImageSource !== ""
                source: root.rightImageSource
                sourceSize: Qt.size(14, 14)
                color: d.contentColor
            }
        }
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }

    Keys.onEnterPressed: root.clicked()
    Keys.onReturnPressed: root.clicked()
}
