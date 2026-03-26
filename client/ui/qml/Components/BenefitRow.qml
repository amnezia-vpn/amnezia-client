import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Controls2/TextTypes"

RowLayout {
    id: root

    property string iconSource: ""
    property string titleText: ""
    property string bodyText: ""
    property bool accent: false

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
                text: root.bodyText
                color: root.accent ? AmneziaStyle.color.goldenApricot : AmneziaStyle.color.mutedGray
                font.pixelSize: 14
                wrapMode: Text.Wrap
            }

            MouseArea {
                anchors.fill: bodyLabel
                visible: root.accent && root.bodyText.length > 0
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var t = root.bodyText.trim()
                    if (t.startsWith("@")) {
                        Qt.openUrlExternally("https://t.me/" + t.substring(1))
                    }
                }
            }
        }
    }
}
