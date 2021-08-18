import QtQuick 2.12
import QtQuick.Controls 2.12

RadioButton {
    id: root

    indicator: Rectangle {
        implicitWidth: 13
        implicitHeight: 13
        x: root.leftPadding
        y: parent.height / 2 - height / 2
        radius: 13
        border.color: root.down ? "#777777" : "#777777"

        Rectangle {
            width: 7
            height: 7
            x: 3
            y: 3
            radius: 4
            color: root.down ? "#15CDCB" : "#15CDCB"
            visible: root.checked
        }
    }

    contentItem: Text {
        text: root.text
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 16
        color: "#181922"
        verticalAlignment: Text.AlignVCenter
        leftPadding: root.indicator.width + root.spacing
    }
    height: 10
}
