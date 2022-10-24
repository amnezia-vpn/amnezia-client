import QtQuick
import QtQuick.Controls

Button {
    id: root
    property bool containsMouse: hovered
    hoverEnabled: true
    flat: true
    highlighted: false

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: false
        cursorShape: Qt.PointingHandCursor
    }
}
