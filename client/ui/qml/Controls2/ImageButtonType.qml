import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: root

    property string image

    property string hoveredColor: Qt.rgba(255, 255, 255, 0.08)
    property string defaultColor: Qt.rgba(255, 255, 255, 0)
    property string pressedColor: Qt.rgba(255, 255, 255, 0.12)

    property string imageColor: "#878B91"

    implicitWidth: 40
    implicitHeight: 40

    hoverEnabled: true

    icon.source: image
    icon.color: imageColor

    background: Rectangle {
        id: background

        anchors.fill: parent
        radius: 12
        color: {
            if (root.enabled) {
                if(root.pressed) {
                    return pressedColor
                }
                return hovered ? hoveredColor : defaultColor
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: false
        cursorShape: Qt.PointingHandCursor
    }
}
