import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"

Item {
    id: root

    property string label
    property string value
    property string copiedText
    property bool showSeparator: true

    implicitWidth: content.implicitWidth + 28
    implicitHeight: Math.max(72, content.implicitHeight + 24)

    RowLayout {
        id: content

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 16
        anchors.rightMargin: 12

        spacing: 0

        ColumnLayout {
            Layout.fillWidth: true

            spacing: 4

            CaptionTextType {
                Layout.fillWidth: true

                text: root.label
            }

            BodyTextType {
                Layout.fillWidth: true

                text: root.value
                color: AmneziaStyle.color.textPrimary
                wrapMode: Text.WrapAnywhere
            }
        }

        IconButtonType {
            icon.source: "qrc:/images/controls/copy.svg"
            Accessible.name: root.label

            onClicked: {
                GC.copyToClipBoard(root.value)
                if (root.copiedText !== "") {
                    PageController.showNotificationMessage(root.copiedText)
                }
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        height: 1
        visible: root.showSeparator
        color: AmneziaStyle.color.borderSoft
    }
}
