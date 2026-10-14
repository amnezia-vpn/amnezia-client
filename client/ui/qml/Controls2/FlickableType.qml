import QtQuick
import QtQuick.Controls

Flickable {
    id: root

    clip: true
    width: parent.width

    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.rightMargin: 1

    ScrollBar.vertical: ScrollBarType {}
}
