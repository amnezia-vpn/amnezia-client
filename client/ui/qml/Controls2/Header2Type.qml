import QtQuick
import QtQuick.Layouts

import "TextTypes"

Item {
    id: root

    property string buttonImage
    property string headerText
    property string descriptionText

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content
        anchors.fill: parent

        ImageButtonType {
            id: backButton

            Layout.leftMargin: -6

            image: root.buttonImage
            imageColor: "#D7D8DB"

            visible: image ? true : false

            onClicked: {
                UiLogic.closePage()
            }
        }

        Header2TextType {
            id: header

            Layout.fillWidth: true

            text: root.headerText
        }

        ParagraphTextType {
            id: description

            Layout.topMargin: 16
            Layout.fillWidth: true

            text: root.descriptionText

            color: "#878B91"

            visible: root.descriptionText !== ""
        }
    }
}
