import QtQuick 2.12
import QtQuick.Controls 2.12
import "../Config"

Flickable {
    id: fl

    clip: true
    width: parent.width

    anchors.topMargin: GC.defaultMargin
    anchors.bottom: parent.bottom
    anchors.bottomMargin: GC.defaultMargin
    anchors.left: root.left
    anchors.leftMargin: GC.defaultMargin
    anchors.right: root.right
    anchors.rightMargin: 1

    Keys.onUpPressed: scrollBar.decrease()
    Keys.onDownPressed: scrollBar.increase()

    ScrollBar.vertical: ScrollBar {
        id: scrollBar
        policy: fl.height >= fl.contentHeight ? ScrollBar.AlwaysOff : ScrollBar.AlwaysOn
    }
}
