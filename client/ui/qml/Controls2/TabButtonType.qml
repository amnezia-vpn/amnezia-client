import QtQuick
import QtQuick.Controls

TabButton {
    id: root

    property string hoveredColor: "#412102"
    property string defaultColor: "#2C2D30"
    property string selectedColor: "#FBB26A"

    property string textColor: "#D7D8DB"

    property bool isSelected: false

    implicitHeight: 48

    hoverEnabled: true

    background: Rectangle {
        id: background

        anchors.fill: parent
        color: "transparent"

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
