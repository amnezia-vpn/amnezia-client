import QtQuick
import QtQuick.Controls

BasicButtonType {
    id: root
    property alias textItem: textItem
    height: 30

    background: Item {}
    contentItem: Item {
        anchors.fill: parent
        SvgImageType {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            svg.source: root.icon.source
            color: "#100A44"
            width: 25
            height: 25
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
