import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.15

Item {
    id: root
    property color color: "#181922"
    property alias svg: image
    Image {
        anchors.fill: parent
        id: image
        sourceSize: Qt.size(root.width, root.height)

        antialiasing: true
        visible: false
    }

    ColorOverlay {
        anchors.fill: image
        source: image
        color: root.color
    }
}
