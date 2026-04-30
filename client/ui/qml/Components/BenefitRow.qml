import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Controls2/TextTypes"

RowLayout {
    id: root

    property string iconSource: ""
    property string titleText: ""
    property string bodyText: ""
    property bool link: false

    readonly property string bodyLineText: root.link && root.bodyText.length > 0 ? "@" + root.bodyText : root.bodyText

    readonly property bool bodyClickable: root.link && root.bodyText.length > 0

    spacing: 12

    Image {
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: 22
        Layout.preferredHeight: 22
        source: root.iconSource
        fillMode: Image.PreserveAspectFit
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 4

        LabelTextType {
            Layout.fillWidth: true
            text: root.titleText
            color: AmneziaStyle.color.paleGray
            font.pixelSize: 16
            font.weight: Font.DemiBold
            wrapMode: Text.Wrap
        }

        Item {
            Layout.fillWidth: true
            implicitHeight: bodyLabel.implicitHeight

            LabelTextType {
                id: bodyLabel
                width: parent.width
                text: root.bodyLineText
                color: root.link ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.mutedGray
                font.pixelSize: 14
                wrapMode: Text.Wrap
            }

            MouseArea {
                anchors.fill: bodyLabel
                visible: root.bodyClickable
                cursorShape: Qt.PointingHandCursor
                onClicked: Qt.openUrlExternally("https://t.me/" + root.bodyText)
            }
        }
    }
}
