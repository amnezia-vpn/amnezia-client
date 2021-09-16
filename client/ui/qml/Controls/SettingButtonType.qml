import QtQuick 2.12
import QtQuick.Controls 2.12

BasicButtonType {
    id: root
    property alias textItem: textItem

    background: Item {}
    contentItem: Item {
        anchors.fill: parent
        Image {
            source: root.icon.source
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            id: textItem
            anchors.fill: parent
            leftPadding: 30
            text: root.text
            color: root.enabled ? "#100A44": "#AAAAAA"
            font.family: "Lato"
            font.styleName: "normal"
            font.pixelSize: 20
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
        }
    }
}
