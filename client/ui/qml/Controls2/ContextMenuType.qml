import QtQuick
import QtQuick.Controls

QtObject {
    id: root

    property var textObj
    property Menu menu: null

    // On Qt < 6.10 the ContextMenu attached type has no native backing on iOS
    // and opens this Qt-drawn menu instead. In that case the native edit menu
    // is presented through UIEditMenuInteraction (IosContextMenu is registered
    // only in iOS builds; on Qt >= 6.10 isAvailable() returns false and the
    // attached type shows the native menu itself).
    readonly property bool useNativeEditMenu: Qt.platform.os === "ios" && typeof IosContextMenu !== "undefined" && IosContextMenu.isAvailable()

    readonly property Component menuComponent: Component {
        Menu {
            id: menu

            property Item inputBlocker: null

            popupType: Popup.Native

            onAboutToShow: {
                if (!root.textObj || !root.textObj.window) {
                    return
                }

                const contentItem = root.textObj.window.contentItem
                if (!menu.inputBlocker) {
                    menu.inputBlocker = inputBlockerComponent.createObject(contentItem)
                } else {
                    menu.inputBlocker.parent = contentItem
                }
            }

            onClosed: {
                if (menu.inputBlocker) {
                    menu.inputBlocker.destroy()
                    menu.inputBlocker = null
                }
            }

            MenuItem {
                text: qsTr("C&ut")
                enabled: root.textObj.selectedText.length > 0
                onTriggered: root.textObj.cut()
            }
            MenuItem {
                text: qsTr("&Copy")
                enabled: root.textObj.selectedText.length > 0
                onTriggered: root.textObj.copy()
            }
            MenuItem {
                text: qsTr("&Paste")
                // Fix calling paste from clipboard when launching app on android/ios
                enabled: (Qt.platform.os === "android" || Qt.platform.os === "ios") ? true : root.textObj.canPaste
                onTriggered: root.textObj.paste()
            }

            MenuItem {
                text: qsTr("&SelectAll")
                enabled: root.textObj.length > 0
                onTriggered: root.textObj.selectAll()
            }
        }
    }

    readonly property Component inputBlockerComponent: Component {
        MouseArea {
            anchors.fill: parent
            preventStealing: true
        }
    }

    function requestNative(position) {
        if (root.useNativeEditMenu && root.textObj) {
            root.textObj.forceActiveFocus()
            IosContextMenu.present(root.textObj, position.x, position.y)
        }
    }

    function request(position) {
        if (root.useNativeEditMenu) {
            root.requestNative(position)
            return
        }

        if (!root.textObj) {
            return
        }

        if (!root.menu) {
            root.menu = root.menuComponent.createObject(root.textObj)
        }
        root.menu.popup(root.textObj, position.x, position.y)
    }
}
