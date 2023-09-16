import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Popup {
    id: root

    modal: false
    closePolicy: Popup.NoAutoClose
    width: 40
    height: 40
    padding: 4

    visible: false

    Overlay.modal: Rectangle {
        color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
    }

    background: Rectangle {
        color: "transparent"
    }

    ImageButtonType {
        image: "qrc:/images/svg/close_black_24dp.svg"
        imageColor: "#D7D8DB"

        implicitWidth: 32
        implicitHeight: 32

        onClicked: {
            PageController.goToDrawerRootPage()
        }
    }
}
