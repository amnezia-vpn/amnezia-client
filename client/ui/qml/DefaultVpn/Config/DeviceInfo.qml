pragma Singleton

import QtQuick

Item {
    readonly property int screenWidth: 380
    readonly property int screenHeight: 680

    function isMobile() {
        if (Qt.platform.os === "android" ||
                Qt.platform.os === "ios") {
            return true
        }
        return false
    }

    function isDesktop() {
        if (Qt.platform.os === "windows" ||
                Qt.platform.os === "linux" ||
                Qt.platform.os === "osx") {
            return true
        }
        return false
    }

    TextEdit {
        id: clipboard
        visible: false
    }

    function copyToClipBoard(text) {
        clipboard.text = text
        clipboard.selectAll()
        clipboard.copy()
        clipboard.select(0, 0)
    }
}
