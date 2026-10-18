import QtQuick
import QtQuick.Controls

ListView {
    id: root

    ScrollBar.vertical: ScrollBarType {}

    clip: true
    reuseItems: true

    interactive: root.orientation === ListView.Vertical
                 ? root.contentHeight + root.topMargin + root.bottomMargin > root.height
                 : root.contentWidth + root.leftMargin + root.rightMargin > root.width

    function findChildWithObjectName(items, name) {
        for (var i = 0; i < items.length; ++i) {
            if (items[i].objectName === name)
                return items[i];
        }
        return null;
    }
}
