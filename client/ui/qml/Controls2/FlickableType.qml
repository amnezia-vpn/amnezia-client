import QtQuick 2.12
import QtQuick.Controls 2.12
import "../Config"

Flickable {
    id: fl

    function ensureVisible(item) {
        if (item.y < fl.contentY) {
            fl.contentY = item.y - 40 // 40 is a top margin
        } else if (item.y + item.height > fl.contentY + fl.height) {
            fl.contentY = item.y + item.height - fl.height + 40 // 40 is a bottom margin
        }
        fl.returnToBounds()
    }

    clip: true
    width: parent.width

    anchors.bottom: parent.bottom
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.rightMargin: 1

    Keys.onUpPressed: scrollBar.decrease()
    Keys.onDownPressed: scrollBar.increase()

    ScrollBar.vertical: ScrollBarType {
        id: scrollBar
        policy: fl.height >= fl.contentHeight ? ScrollBar.AlwaysOff : ScrollBar.AlwaysOn
    }
}
