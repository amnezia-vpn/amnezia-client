import QtQuick
import QtQuick.Layouts

import "TextTypes"

Item {
    id: root

    property string actionButtonImage
    property var actionButtonFunction

    property string headerText
    property string descriptionText

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content
        anchors.fill: parent

        RowLayout {
            Header2TextType {
                id: header

                Layout.fillWidth: true

                text: root.headerText
            }

            ImageButtonType {
                id: headerActionButton

                implicitWidth: 40
                implicitHeight: 40

                image: root.actionButtonImage
                imageColor: "#D7D8DB"

                visible: image ? true : false

                onClicked: {
                    if (actionButtonFunction && typeof actionButtonFunction === "function") {
                        actionButtonFunction()
                    }
                }
            }
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
