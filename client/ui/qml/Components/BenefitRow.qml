import QtQuick
import QtQuick.Layouts

import Style 1.0

import "../Controls2/TextTypes"

RowLayout {
    id: root

    property string iconSource: ""
    property string titleText: ""
    property string bodyText: ""

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

        LabelTextType {
            Layout.fillWidth: true
            text: root.bodyText
            color: AmneziaStyle.color.mutedGray
            font.pixelSize: 14
            wrapMode: Text.Wrap
        }
    }
}
