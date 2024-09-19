import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

import Style 1.0

FocusScope {
    id: root

    property string backButtonImage: "qrc:/images/controls/arrow-left.svg"
    property var backButtonFunction

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    visible: backButtonImage !== ""

    RowLayout {
        id: content

        anchors.fill: parent
        anchors.leftMargin: 8

        ImageButtonType {
            id: backButton
            image: backButtonImage
            imageColor: AmneziaStyle.color.paleGray

            implicitWidth: 40
            implicitHeight: 40

            onClicked: {
                if (backButtonFunction && typeof backButtonFunction === "function") {
                    backButtonFunction()
                } else {
                    PageController.closePage()
                }
            }
        }

        Rectangle {
            id: background
            Layout.fillWidth: true

            color: AmneziaStyle.color.transparent
        }
    }

    Keys.onEnterPressed: backButton.clicked()
    Keys.onReturnPressed: backButton.clicked()
}
