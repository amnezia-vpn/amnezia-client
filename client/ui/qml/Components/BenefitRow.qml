import QtQuick
import QtQuick.Controls.impl
import QtQuick.Layouts

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

RowLayout {
    id: root

    property string iconSource
    property string title
    property string subtitle
    property bool isLink: false

    spacing: 16

    IconImage {
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: 24
        Layout.preferredHeight: 24

        source: root.iconSource
        sourceSize: Qt.size(24, 24)
        color: AmneziaStyle.color.iconDefault
    }

    ColumnLayout {
        Layout.fillWidth: true

        spacing: 4

        H3TextType {
            Layout.fillWidth: true

            text: root.title
        }

        SmallTextType {
            Layout.fillWidth: true

            visible: !root.isLink && text !== ""
            text: root.subtitle
        }

        LinkButtonType {
            Layout.alignment: Qt.AlignLeft
            Layout.maximumWidth: parent.width

            visible: root.isLink && root.subtitle !== ""
            text: "@" + root.subtitle

            onClicked: Qt.openUrlExternally("https://t.me/" + root.subtitle)
        }
    }
}
