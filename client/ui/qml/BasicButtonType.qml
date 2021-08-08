import QtQuick 2.12
import QtQuick.Controls 2.12

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
