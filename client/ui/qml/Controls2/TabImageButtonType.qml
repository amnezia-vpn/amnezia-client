import QtQuick
import QtQuick.Controls

TabButton {
    id: root

    property string hoveredColor: "#412102"
    property string defaultColor: "#D7D8DB"
    property string selectedColor: "#FBB26A"

    property string image

    property bool isSelected: false

    hoverEnabled: true

    icon.source: image
    icon.color: isSelected ? selectedColor : defaultColor

    background: Rectangle {
        anchors.fill: parent
        color: "transparent"
    }
}
