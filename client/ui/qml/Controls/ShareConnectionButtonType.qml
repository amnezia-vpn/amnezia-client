import QtQuick 2.12
import QtQuick.Controls 2.12


BasicButtonType {
    id: root
    background: Rectangle {
        anchors.fill: parent
        radius: 4
        color: root.containsMouse ? "#282932" : "#181922"
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
