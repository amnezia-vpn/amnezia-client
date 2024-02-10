import QtQuick
import QtQuick.Controls

TabButton {
    id: root

    property string hoveredColor: "#633303"
    property string defaultColor: "#D7D8DB"
    property string selectedColor: "#FBB26A"

    property string image

    property bool isSelected: false

    hoverEnabled: true

    icon.source: image
    icon.color: isSelected ? selectedColor : defaultColor

    background: Rectangle {
        id: background
        anchors.fill: parent
        color: "transparent"
    }

    MouseArea {
        anchors.fill: background
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }
}
