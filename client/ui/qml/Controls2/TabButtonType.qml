import QtQuick
import QtQuick.Controls

TabButton {
    id: root

    property string hoveredColor: "#633303"
    property string defaultColor: "#2C2D30"
    property string selectedColor: "#FBB26A"

    property string textColor: "#D7D8DB"

    property string borderFocusedColor: "#D7D8DB"
    property int borderFocusedWidth: 1

    property bool isSelected: false

    implicitHeight: 48

    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    background: Rectangle {
        id: background

        anchors.fill: parent
        color: "transparent"

        border.color: root.activeFocus ? root.borderFocusedColor : "transparent"
        border.width: root.activeFocus ? root.borderFocusedWidth : 0

        Rectangle {
            width: parent.width
            height: 1
            y: parent.height - height
            color: {
                if(root.isSelected) {
                    return selectedColor
                }
                return hovered ? hoveredColor : defaultColor
            }

            Behavior on color {
                PropertyAnimation { duration: 200 }
            }
        }
    }

    MouseArea {
        anchors.fill: background
        cursorShape: Qt.PointingHandCursor
        enabled: false
    }

    contentItem: Text {
        anchors.fill: background
        height: 24

        font.family: "Noto Sans"
        font.styleName: "normal"
        font.weight: 500
        font.pixelSize: 16
        color: textColor
        text: root.text

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
