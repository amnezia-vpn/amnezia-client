import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0

import "TextTypes"

Rectangle {
    id: root

    property string textColor: AmneziaStyle.color.paleGray
    property string backGroundColor: AmneziaStyle.color.onyxBlack
    property string imageColor: AmneziaStyle.color.paleGray
    property string textString
    property int textFormat: Text.PlainText

    property string iconPath
    property real iconWidth: 16
    property real iconHeight: 16
    property real iconSpacing: 8

    property real verticalPadding: 8
    property real textPixelSize: 13
    property real textLineHeight: 16

    color: backGroundColor
    radius: 8
    implicitHeight: content.implicitHeight + content.anchors.topMargin + content.anchors.bottomMargin

    RowLayout {
        id: content
        width: parent.width
        anchors.fill: parent

        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.topMargin: root.verticalPadding
        anchors.bottomMargin: root.verticalPadding

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
            Layout.leftMargin: root.iconSpacing

            lineHeight: root.textLineHeight + LanguageUiController.getLineHeightAppend()
            font.pixelSize: root.textPixelSize

            text: textString
            textFormat: root.textFormat
            color: textColor
        }
    }
}
