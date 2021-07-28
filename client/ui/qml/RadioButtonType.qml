import QtQuick 2.12
import QtQuick.Controls 2.12

RadioButton {
    id: root
    contentItem: Text {
        text: root.text
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 16
        color: "#181922"
        verticalAlignment: Text.AlignVCenter
        leftPadding: root.indicator.width + root.spacing
    }
}
