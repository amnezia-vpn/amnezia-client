import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root

    implicitHeight: 190
    implicitWidth: 190

    background: Rectangle {
        id: background

        radius: parent.width * 0.5

        color: "transparent"

        border.width: 2
        border.color: "white"
    }

    contentItem: Text {
        height: 24

        font.family: "PT Root UI VF"
        font.weight: 700
        font.pixelSize: 20

        color: "#D7D8DB"
        text: root.text

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    onClicked: {
        background.color = "red"
        ConnectionController.onConnectionButtonClicked()
    }
}
