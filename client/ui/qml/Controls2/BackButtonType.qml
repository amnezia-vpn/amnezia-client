import QtQuick
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects

Item {
    id: root

    property string backButtonImage: "qrc:/images/controls/arrow-left.svg"
    property var backButtonFunction

    implicitWidth: content.implicitWidth
    implicitHeight: content.implicitHeight

    visible: backButtonImage !== ""

    onActiveFocusChanged: {
        if (activeFocus) {
            backButton.forceActiveFocus()
        }
    }

    RowLayout {
        id: content

        anchors.fill: parent
        anchors.leftMargin: 8

        ImageButtonType {
            id: backButton
            image: backButtonImage
            imageColor: "#D7D8DB"

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

            color: "transparent"
        }
    }

    Keys.onEnterPressed: backButton.clicked()
    Keys.onReturnPressed: backButton.clicked()
}
