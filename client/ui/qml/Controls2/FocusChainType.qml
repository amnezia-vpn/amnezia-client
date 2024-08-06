import QtQuick

import "../Config"
import "../Components"

FocusScope {
    id: root

    required property var defaultActiveFocusItem

    property var focusChain: []

    onActiveFocusChanged: {
        // console.log("root activeFocusChanged=-=--=-=-=-=--", activeFocus)
    }

    function getChildren(parent) {
        for (var i = 0; i < parent.children.length; i++) {
            var child = parent.children[i]
            // console.log(child)

            if (child.children) {
                getChildren(child)
            }
        }
    }

    function isFocusableItem(item) {
        // if (!item) { return false }

        return item instanceof BasicButtonType ||
                item instanceof ConnectButton ||
                item instanceof LabelWithButtonType ||
                item instanceof SwitcherType ||
                item instanceof ImageButtonType ||
                item instanceof BackButtonType ||
                item instanceof DrawerType2 && !item.isOpened && item.collapsedContent
    }

    function getFocusChain(parent) {
        let focusChain = []
        // console.log("-------------->", parent.children.length)
        // console.log(parent.children)
        // console.log("-------------->")
        for (var i = 0; i < parent.children.length; i++) {
            var child = parent.children[i]

            if (
                    child instanceof DropDownType ||
                    child instanceof HomeSplitTunnelingDrawer ||
                    child instanceof ConnectionTypeSelectionDrawer
                    ) {
                continue
            }
            // console.log(child, isFocusableItem(child), child.objectName)
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
        root.focusChain = getFocusChain(root)
        console.log("focusChain------------------>", focusChain)
        for (let i = 0; i < focusChain.length; i++) {
            focusChain[i].KeyNavigation.tab = focusChain[(i + 1) % focusChain.length]
            focusChain[i].KeyNavigation.backtab = focusChain[(i + focusChain.length - 1) % focusChain.length]
        }

        const lastItem = focusChain[focusChain.length - 1]
        lastItem.Keys.onTabPressed.connect(function() {
            lastItemTabClicked(lastItem)
        })
    }



    Item {
        id: focusItem
        focus: true
        // KeyNavigation.tab: defaultActiveFocusItem ? defaultActiveFocusItem : focusItem

        Keys.onTabPressed: {
            // console.log("tab pressed", defaultActiveFocusItem)
            defaultActiveFocusItem.forceActiveFocus()
        }

        onActiveFocusChanged: {
            // console.log("focusItem activeFocusChanged")

        }
    }


    function lastItemTabClicked(lastItem = null) {
        console.log("lastItemTabClicked", lastItem)
        if (GC.isMobile()) {
            return
        }

        focusItem.forceActiveFocus()
        PageController.forceTabBarActiveFocus()

        if (lastItem && lastItem.parentFlickable) {
            console.log("lastItemTabClicked", lastItem.parentFlickable)
            lastItem.parentFlickable.contentY = 0
        }

        // if (focusItem) {
        //     focusItem.forceActiveFocus()
        //     PageController.forceTabBarActiveFocus()
        // } else {
        //     if (defaultActiveFocusItem) {
        //         defaultActiveFocusItem.forceActiveFocus()
        //     }
        //     PageController.forceTabBarActiveFocus()
        // }
    }


}
