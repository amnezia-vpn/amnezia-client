import QtQuick
import QtQuick.Controls

Menu {
    property var textObj

    popupType: Popup.Native

    // On Qt < 6.10 the ContextMenu attached type has no native backing on iOS
    // and opens this Qt-drawn menu instead. In that case the native edit menu
    // is presented through UIEditMenuInteraction (IosContextMenu is registered
    // only in iOS builds; on Qt >= 6.10 isAvailable() returns false and the
    // attached type shows the native menu itself).
    readonly property bool useNativeEditMenu: typeof IosContextMenu !== "undefined" && IosContextMenu.isAvailable()

    function requestNative(position) {
        if (useNativeEditMenu && textObj) {
            textObj.forceActiveFocus()
            IosContextMenu.present(textObj, position.x, position.y)
        }
    }

    property Item inputBlocker: null

    Component {
        id: inputBlockerComponent

        MouseArea {
            anchors.fill: parent
            preventStealing: true
        }
    }

    onAboutToShow: {
        if (!textObj || !textObj.window) {
            return
        }

        const contentItem = textObj.window.contentItem
        if (!inputBlocker) {
            inputBlocker = inputBlockerComponent.createObject(contentItem)
        } else {
            inputBlocker.parent = contentItem
        }
    }

    onClosed: {
        if (inputBlocker) {
            inputBlocker.destroy()
            inputBlocker = null
        }
    }

    MenuItem {
        text: qsTr("C&ut")
        enabled: textObj.selectedText
        onTriggered: textObj.cut()
    }
    MenuItem {
        text: qsTr("&Copy")
        enabled: textObj.selectedText
        onTriggered: textObj.copy()
    }
    MenuItem {
        text: qsTr("&Paste")
        // Fix calling paste from clipboard when launching app on android/ios
        enabled: (Qt.platform.os === "android" || Qt.platform.os === "ios") ? true : textObj.canPaste
        onTriggered: textObj.paste()
    }

    MenuItem {
        text: qsTr("&SelectAll")
        enabled: textObj.length > 0
        onTriggered: textObj.selectAll()
    }
}
