import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import "TextTypes"

Rectangle {
    id: root

    property string textColor: "#D7D8DB"
    property string backGroundColor: "#1C1D21"
    property string imageColor: "#D7D8DB"
    property string textString
    property int textFormat: Text.PlainText

    property string iconPath
    property real iconWidth: 16
    property real iconHeight: 16

    color: backGroundColor
    radius: 8
    implicitHeight: content.implicitHeight + content.anchors.topMargin + content.anchors.bottomMargin

    RowLayout {
        id: content
        width: parent.width
        anchors.fill: parent

        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: 8
        anchors.bottomMargin: 8

        spacing: 0

        Image {
            Layout.alignment: Qt.AlignTop

            width: iconWidth
            height: iconHeight

            source: iconPath

            layer {
                enabled: true
                effect: ColorOverlay {
                    color: imageColor
                }
            }
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
