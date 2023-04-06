import QtQuick
import "../Config"

Text {
    id: root
    width: parent.width - 2 * GC.defaultMargin
    anchors.topMargin: 10

    font.family: "Lato"
    font.styleName: "normal"
    font.pixelSize: 16
    color: "#181922"
    horizontalAlignment: Text.AlignLeft
    verticalAlignment: Text.AlignVCenter
    wrapMode: Text.Wrap
}

