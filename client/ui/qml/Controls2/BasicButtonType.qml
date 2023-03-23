import QtQuick
import QtQuick.Controls

Button {
    id: root

    property string hoveredColor: "#C1C2C5"
    property string defaultColor: "#D7D8DB"
    property string disabledColor: "#494B50"
    property string pressedColor: "#979799"

    property string textColor: "#0E0E11"

    property string borderColor: "#D7D8DB"
    property int borderWidth: 0

    implicitWidth: 328
    implicitHeight: 56

    hoverEnabled: true

    background: Rectangle {
        id: background
        anchors.fill: parent
        radius: 16
        color: {
            if (root.enabled) {
                if(root.pressed) {
                    return pressedColor
                }
                return hovered ? hoveredColor : defaultColor
            } else {
                return disabledColor
            }
        }
        border.color: borderColor
        border.width: borderWidth
    }

    MouseArea {
        anchors.fill: background
        enabled: false
        cursorShape: Qt.PointingHandCursor
    }

    contentItem: Text {
        anchors.fill: background
        font.family: "PT Root UI"
        font.styleName: "normal"
        font.weight: 400
        font.pixelSize: 16
        color: textColor
        text: root.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
