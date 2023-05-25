import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import "TextTypes"

Popup {
    id: root

    property string popupErrorMessageText
    property bool closeButtonVisible: true

    leftMargin: 25
    rightMargin: 25
    bottomMargin: 70

    width: parent.width - leftMargin - rightMargin

    anchors.centerIn: parent
    modal: true
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle {
        color: Qt.rgba(14/255, 14/255, 17/255, 0.8)
    }

    background: Rectangle {
        anchors.fill: parent

        color: Qt.rgba(215/255, 216/255, 219/255, 0.95)
        radius: 4
    }

    contentItem: RowLayout {
        width: parent.width

        CaptionTextType {
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true

            text: root.popupErrorMessageText
        }

        BasicButtonType {
            visible: closeButtonVisible

            defaultColor: Qt.rgba(215/255, 216/255, 219/255, 0.95)
            hoveredColor: "#C1C2C5"
            pressedColor: "#AEB0B7"
            disabledColor: "#494B50"

            textColor: "#0E0E11"
            borderWidth: 0

            text: "Close"
            onClicked: {
                root.close()
            }
        }
    }
}
