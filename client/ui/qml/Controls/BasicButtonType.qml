import QtQuick
import QtQuick.Controls

Button {
    id: root
    hoverEnabled: true
    property bool containsMouse: hovered
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: false
        cursorShape: Qt.PointingHandCursor
    }
}
