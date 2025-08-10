import QtQuick
import QtQuick.Controls

ListView {
    id: root

    property bool isFocusable: true

    ScrollBar.vertical: ScrollBarType {}

    clip: true
    reuseItems: true

    function findChildWithObjectName(items, name) {
        for (var i = 0; i < items.length; ++i) {
            if (items[i].objectName === name)
                return items[i];
        }
        return null;
    }
}
