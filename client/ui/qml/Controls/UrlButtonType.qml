import QtQuick 2.12
import QtQuick.Controls 2.12

BasicButtonType {
    property alias label: lbl
    id: root
    antialiasing: true
    height: 21
    background: Item {}

    contentItem: Text {
        id: lbl
        anchors.fill: parent
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 18
        font.underline: true

        text: root.text
        color: "#3045ee"

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
