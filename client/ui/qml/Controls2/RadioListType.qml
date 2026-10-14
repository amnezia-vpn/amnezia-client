import QtQuick
import QtQuick.Controls

ListViewType {
    id: root

    property int selectedIndex: 0
    property string selectedText
    property string currentValue: ""

    signal activated(int index)

    implicitHeight: contentHeight

    leftMargin: 16
    rightMargin: 16

    function triggerCurrentItem() {
        const item = root.itemAtIndex(root.selectedIndex)
        if (item) {
            item.click()
        }
    }

    ButtonGroup {
        id: buttonGroup
    }

    delegate: RadioItemType {
        required property int index
        required property string name

        width: root.width - root.leftMargin - root.rightMargin

        text: name
        showSeparator: index < root.count - 1

        ButtonGroup.group: buttonGroup
        checked: root.currentValue !== "" ? name === root.currentValue : root.selectedIndex === index

        onClicked: {
            root.selectedIndex = index
            root.selectedText = name
            root.activated(index)
        }

        Component.onCompleted: {
            if (root.selectedIndex === index) {
                root.selectedText = name
            }
        }
    }
}
