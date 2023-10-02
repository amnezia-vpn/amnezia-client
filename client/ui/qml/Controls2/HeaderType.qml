import QtQuick
import QtQuick.Layouts

import "TextTypes"

Item {
    id: root

    property string actionButtonImage
    property var actionButtonFunction

    property string headerText
    property int headerTextMaximumLineCount: 2
    property int headerTextElide: Qt.ElideRight

    property string descriptionText

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    ColumnLayout {
        id: content
        anchors.fill: parent

        RowLayout {
            Header1TextType {
                id: header

                Layout.fillWidth: true

                text: root.headerText
                maximumLineCount: root.headerTextMaximumLineCount
                elide: root.headerTextElide
            }

            ImageButtonType {
                id: headerActionButton

                implicitWidth: 40
                implicitHeight: 40

                Layout.alignment: Qt.AlignRight

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
