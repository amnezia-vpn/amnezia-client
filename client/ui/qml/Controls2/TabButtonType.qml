import QtQuick
import QtQuick.Controls

import Style 1.0

TabButton {
    id: root

    property string hoveredColor: AmneziaStyle.color.brown
    property string defaultColor: AmneziaStyle.color.greyDark
    property string selectedColor: AmneziaStyle.color.orange

    property string textColor: AmneziaStyle.color.white

    property string borderFocusedColor: AmneziaStyle.color.white
    property int borderFocusedWidth: 1

    property bool isSelected: false

    implicitHeight: 48

    hoverEnabled: true
    focusPolicy: Qt.TabFocus

    background: Rectangle {
        id: background

        anchors.fill: parent
        color: AmneziaStyle.color.transparent

        border.color: root.activeFocus ? root.borderFocusedColor : AmneziaStyle.color.transparent
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

        font.family: "PT Root UI VF"
        font.styleName: "normal"
        font.weight: 500
        font.pixelSize: 16
        color: textColor
        text: root.text

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
