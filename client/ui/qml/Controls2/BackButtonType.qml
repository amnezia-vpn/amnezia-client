import QtQuick

FocusScope {
    id: root

    property string backButtonImage: "qrc:/images/controls/arrow-left.svg"
    property var backButtonFunction

    implicitWidth: backButton.implicitWidth + backButton.x
    implicitHeight: backButton.implicitHeight

    visible: backButtonImage !== ""

    IconButtonType {
        id: backButton

        x: 16
        anchors.verticalCenter: parent.verticalCenter

        icon.source: root.backButtonImage

        onClicked: {
            if (root.backButtonFunction && typeof root.backButtonFunction === "function") {
                root.backButtonFunction()
            } else {
                PageController.closePage()
            }
        }
    }
}
