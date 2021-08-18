import QtQuick 2.12
import QtQuick.Controls 2.12

BasicButtonType {
    id: root
    property alias iconMargin: img.anchors.margins
    background: Item {}
    contentItem: Image {
        id: img
        source: root.icon.source
        anchors.fill: root
        anchors.margins: root.containsMouse ? 3 : 4
    }
}
