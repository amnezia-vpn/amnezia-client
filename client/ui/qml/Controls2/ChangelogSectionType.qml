import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.impl

import Style 1.0

import "TextTypes"

ColumnLayout {
    id: root

    property string iconSource
    property string title
    property var items: []

    spacing: 10

    visible: items && items.length > 0

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        IconImage {
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18

            source: root.iconSource
            sourceSize: Qt.size(18, 18)
            color: AmneziaStyle.color.statusSuccess
        }

        H3TextType {
            font.weight: 600
            Layout.fillWidth: true

            text: root.title
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Repeater {
            model: root.items

            delegate: BodyTextType {
                Layout.fillWidth: true

                color: AmneziaStyle.color.textTertiary

                text: modelData
            }
        }
    }
}
