import QtQuick 2.12
import QtQuick.Controls 2.12

BasicButtonType {
    id: root
    property alias iconMargin: img.anchors.margins
    property alias img: img
    property int imgMargin: 4
    property int imgMarginHover: 3
    background: Item {}
    contentItem: Image {
        id: img
        source: root.icon.source
        anchors.fill: root
        anchors.margins: root.containsMouse ? imgMarginHover : imgMargin
    }
}
