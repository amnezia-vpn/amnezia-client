import QtQuick
import QtQuick.Controls


BasicButtonType {
    id: root
    width: parent.width - 80
    height: 40

    background: Rectangle {
        anchors.fill: parent
        radius: 4
        color: root.enabled ? (root.containsMouse ? "#211966" : "#100A44") : "#888888"
    }
    font.pixelSize: 16
    contentItem: Text {
        anchors.fill: parent
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: root.font.pixelSize
        color: "#D4D4D4"
        text: root.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    antialiasing: true
}
