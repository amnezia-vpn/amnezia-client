import QtQuick
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "TextTypes"

Item {
    id: root

    property string iconSource: "qrc:/images/controls/search.svg"
    property color iconColor: AmneziaStyle.color.iconSecondary
    property string title
    property string description

    default property alias actions: actionsRow.data

    implicitWidth: content.implicitWidth + 32
    implicitHeight: content.implicitHeight + 64

    ColumnLayout {
        id: content

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -32
        width: Math.min(parent.width - 32, 358)

        spacing: 20

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter

            spacing: 2

            IconImage {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 50
                Layout.preferredHeight: 50

                visible: root.iconSource !== ""
                source: root.iconSource
                sourceSize: Qt.size(50, 50)
                color: root.iconColor
            }

            ColumnLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: Math.min(280, content.width)

                spacing: 6

                H3TextType {
                    Layout.fillWidth: true

                    visible: text !== ""
                    text: root.title
                    horizontalAlignment: Text.AlignHCenter
                }

                SmallTextType {
                    Layout.fillWidth: true

                    visible: text !== ""
                    text: root.description
                    color: AmneziaStyle.color.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }

        RowLayout {
            id: actionsRow

            Layout.alignment: Qt.AlignHCenter

            spacing: 16
            visible: children.length > 0
        }
    }
}
