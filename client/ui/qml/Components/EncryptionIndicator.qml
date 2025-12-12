import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0

import "../Controls2"
import "../Controls2/TextTypes"

import "../Config"

Rectangle {
    id: root

    property string textColor: AmneziaStyle.color.paleGray

    property string textString
    property int textFormat: Text.PlainText

    property string iconPath
    property real iconWidth: 16
    property real iconHeight: 16

    color: AmneziaStyle.color.onyxBlack
    radius: 32
    implicitHeight: iconHeight + 8

    Layout.fillWidth: true
    Layout.leftMargin: 16
    Layout.rightMargin: 16
    Layout.bottomMargin: 16

    RowLayout {
        id: content
        width: parent.width
        anchors.fill: parent

        anchors.leftMargin: content.width / 4
        anchors.rightMargin: content.width / 4
        anchors.topMargin: 4
        anchors.bottomMargin: 4

        spacing: 0

        Image {
            Layout.alignment: Qt.AlignTop

            width: iconWidth
            height: iconHeight

            source: iconPath
        }

        CaptionTextType {
            id: supportingText

            Layout.fillWidth: true
            Layout.leftMargin: 8

            text: textString
            textFormat: root.textFormat
            color: textColor
        }
    }
}