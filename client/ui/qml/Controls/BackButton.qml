import QtQuick
import QtQuick.Controls

Button {
    id: root
    x: 10
    y: 5
    width: 41
    height: 35

    hoverEnabled: true
    property bool containsMouse: hovered

    background: Item {}

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: false
        cursorShape: Qt.PointingHandCursor
    }

    onClicked: {
        UiLogic.closePage()
    }

    contentItem: Image {
        id: img
        source: "qrc:/images/arrow_left.png"
        anchors.fill: root
        anchors.margins: root.containsMouse ? 9 : 10
    }
}
