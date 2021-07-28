import QtQuick 2.12
import QtQuick.Controls 2.12

Button {
    id: root
    property alias containsMouse: mouseArea.containsMouse
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
    }
}
