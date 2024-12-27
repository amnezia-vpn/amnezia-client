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
        color: AmneziaStyle.color.translucentMidnightBlack
    }

    background: Rectangle {
        color: AmneziaStyle.color.transparent
    }

    ImageButtonType {
        id: button

        image: "qrc:/images/svg/close_black_24dp.svg"
        imageColor: AmneziaStyle.color.paleGray

        implicitWidth: 40
        implicitHeight: 40

        onClicked: {
            PageController.goToDrawerRootPage()
        }
    }
}
