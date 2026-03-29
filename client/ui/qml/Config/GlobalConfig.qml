pragma Singleton
import QtQuick

Item {
    readonly property string screenHome: "qrc:/ScreenHome.qml"
    readonly property string screenHomeIntroGifEx1: "qrc:/ScreenHomeIntroGifEx1.qml"

    readonly property int screenWidth: 380
    readonly property int screenHeight: 680
    readonly property int compactBreakpoint: 720
    readonly property int mediumBreakpoint: 1100
    readonly property int desktopPreferredWidth: 560
    readonly property int desktopPreferredHeight: 820
    readonly property int desktopMinWidth: 400
    readonly property int desktopMinHeight: 720

    readonly property int defaultMargin: 20

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

    function isCompactWidth(width) {
        return width < compactBreakpoint
    }

    function isMediumWidth(width) {
        return width >= compactBreakpoint && width < mediumBreakpoint
    }

    function isWideWidth(width) {
        return width >= mediumBreakpoint
    }

    function pageHorizontalMargin(width) {
        if (isWideWidth(width)) {
            return 28
        }
        if (isMediumWidth(width)) {
            return 22
        }
        return 16
    }

    function pageMaxWidth(width) {
        if (isWideWidth(width)) {
            return 1160
        }
        if (isMediumWidth(width)) {
            return 920
        }
        return Math.max(0, width - pageHorizontalMargin(width) * 2)
    }

    TextEdit{
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
