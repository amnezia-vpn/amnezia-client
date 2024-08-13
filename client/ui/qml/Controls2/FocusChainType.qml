import QtQuick

import "../Config"
import "../Components"

FocusScope {
    id: root

    property bool focusTabBarOnLastItem: true
    property alias focusItem_: focusItem

    QtObject {
        id: internal

        property var firstActiveFocusItem: null
        property var focusChain: []
    }

    onVisibleChanged: {
        focusItem_.focus = true
    }

    function getChildren(parent) {
        for (var i = 0; i < parent.children.length; i++) {
            var child = parent.children[i]

            if (child.children) {
                getChildren(child)
            }
        }
    }

    function isFocusableItem(item) {
        if (!item || !item.visible || !item.enabled) {
            return false
        }

        return item instanceof BasicButtonType ||
                item instanceof ConnectButton ||
                item instanceof LabelWithButtonType ||
                item instanceof SwitcherType ||
                item instanceof ImageButtonType ||
                item instanceof TextFieldWithHeaderType ||
                item instanceof BackButtonType ||
                item instanceof HorizontalRadioButton ||
                item instanceof DrawerType2 && !item.isOpened && item.collapsedContent
    }

    function getFocusChain(parent) {
        let focusChain = []
        for (var i = 0; i < parent.children.length; i++) {
            var child = parent.children[i]

            if (child instanceof DropDownType ||
                    child instanceof HomeSplitTunnelingDrawer ||
                    child instanceof ConnectionTypeSelectionDrawer
                    ) {
                continue
            }

            if (isFocusableItem(child)) {
                focusChain.push(child)

                continue
            }

            if (child.children) {
                focusChain = focusChain.concat(getFocusChain(child))
            }
        }

        return focusChain
    }

    Component.onCompleted: {
        internal.focusChain = getFocusChain(root)

        if (internal.focusChain.length === 0) {
            return
        }

        for (let i = 0; i < internal.focusChain.length; i++) {
            internal.focusChain[i].KeyNavigation.tab = internal.focusChain[(i + 1) % internal.focusChain.length]
            internal.focusChain[i].KeyNavigation.backtab = internal.focusChain[(i + internal.focusChain.length - 1) % internal.focusChain.length]
        }

        internal.firstActiveFocusItem = internal.focusChain[0]

        const lastItem = internal.focusChain[internal.focusChain.length - 1]
        lastItem.Keys.onTabPressed.connect(function() {
            lastItemTabClicked(lastItem)
        })

        for (let j = 0; j < internal.focusChain.length; j++) {
            if (internal.focusChain[j] instanceof TextFieldWithHeaderType) {
                internal.focusChain[j].forceActiveFocus()
                break
            }
        }
    }

    Item {
        id: focusItem
        focus: true

        Keys.onTabPressed: {
            if (internal.firstActiveFocusItem) {
                internal.firstActiveFocusItem.focus = true
                internal.firstActiveFocusItem.forceActiveFocus()
            }
        }
    }


    function lastItemTabClicked() {
        focusItem.focus = true

        if (GC.isMobile()) {
            return
        }

        if (focusTabBarOnLastItem) {
            PageController.forceTabBarActiveFocus()
        }


        if (lastItem && lastItem.parentFlickable) {
            lastItem.parentFlickable.contentY = 0
        }
    }
}
