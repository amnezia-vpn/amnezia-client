import QtQuick
import QtQuick.Controls

ListViewType {
    id: root

    property int selectedIndex: 0
    readonly property string selectedText: root.textAt(root.selectedIndex)
    property string currentValue: ""

    signal activated(int index)

    implicitHeight: contentHeight

    leftMargin: 16
    rightMargin: 16

    function textAt(index) {
        if (!root.model || !root.model.get || index < 0 || index >= root.count) {
            return ""
        }
        const item = root.model.get(index)
        return item ? item.name : ""
    }

    function select(index) {
        root.selectedIndex = index
        root.activated(index)
    }

    delegate: RadioItemType {
        required property int index
        required property string name

        width: root.width - root.leftMargin - root.rightMargin

        text: name
        showSeparator: index < root.count - 1

        checked: root.currentValue !== "" ? name === root.currentValue : root.selectedIndex === index

        onClicked: root.select(index)
    }
}
