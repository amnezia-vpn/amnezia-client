import QtQuick
import QtQuick.Controls

Flickable {
    id: root

    clip: true
    width: parent.width

    interactive: root.contentHeight + root.topMargin + root.bottomMargin > root.height

    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right

    ScrollBar.vertical: ScrollBarType {}
}
