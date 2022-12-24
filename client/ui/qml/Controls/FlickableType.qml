import QtQuick 2.12
import QtQuick.Controls 2.12

Flickable {
    id: fl

    Keys.onUpPressed: scrollBar.decrease()
    Keys.onDownPressed: scrollBar.increase()

    ScrollBar.vertical: ScrollBar {
        id: scrollBar
        policy: fl.height > fl.contentHeight ? ScrollBar.AlwaysOff : ScrollBar.AlwaysOn
    }
}
