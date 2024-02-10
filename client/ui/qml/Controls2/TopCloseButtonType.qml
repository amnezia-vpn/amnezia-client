import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Popup {
    id: root

    property alias buttonWidth: button.implicitWidth

    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 4

    visible: false

    Overlay.modal: Rectangle {
        color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
    }

    background: Rectangle {
        color: "transparent"
    }

    ImageButtonType {
        id: button

        image: "qrc:/images/svg/close_black_24dp.svg"
        imageColor: "#D7D8DB"

        implicitWidth: 40
        implicitHeight: 40

        onClicked: {
            PageController.goToDrawerRootPage()
        }
    }
}
