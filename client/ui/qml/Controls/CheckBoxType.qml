import QtQuick 2.12
import QtQuick.Controls 2.12

CheckBox {
    id: root
    property int imageWidth : 20
    property int imageHeight : 20
    indicator: Image {
        id: indicator
        anchors.verticalCenter: root.verticalCenter
        height: imageHeight
        width: imageWidth
        source: root.checked ? "qrc:/images/controls/check_on.png"
                             : "qrc:/images/controls/check_off.png"
    }

    contentItem: Text {
        text: root.text
        font.family: "Lato"
        font.styleName: "normal"
        font.pixelSize: 16
        color: "#181922"
        verticalAlignment: Text.AlignVCenter
        leftPadding: root.indicator.width + root.spacing
        wrapMode: Text.Wrap
    }
}
