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

    property bool linkEnabled: false

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
        anchors.centerIn: parent

        spacing: 0

        Image {
            width: root.iconWidth
            height: root.iconHeight

            source: root.iconPath
        }

        CaptionTextType {
            id: supportingText

            Layout.fillWidth: true
            Layout.leftMargin: 8

            text: root.textString
            textFormat: Text.RichText
            color: root.textColor
        }

        BasicButtonType {
            visible: root.linkEnabled

            hoverEnabled: false

            implicitHeight: root.iconHeight
            implicitWidth: linkText.width/2 + 8

            defaultColor: AmneziaStyle.color.transparent

            CaptionTextType {
                id: linkText

                leftPadding: 4

                width: linkText.text.length * linkText.font.pixelSize

                text: qsTr("Learn more")
                textFormat: Text.RichText
                color: AmneziaStyle.color.goldenApricot
            }

            clickedFunc: function() {
                Qt.openUrlExternally("https://storage.googleapis.com/amnezia/docs?m-path=/documentation/instructions/encryption")
            }
        }
    }
}