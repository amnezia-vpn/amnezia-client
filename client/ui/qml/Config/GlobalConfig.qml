pragma Singleton
import QtQuick

Item {
    readonly property string screenHome: "qrc:/ScreenHome.qml"
    readonly property string screenHomeIntroGifEx1: "qrc:/ScreenHomeIntroGifEx1.qml"

    readonly property int screenWidth: 380
    readonly property int screenHeight: 640

    function isMobile() {
        if (Qt.platform.os == "android" ||
                Qt.platform.os == "ios") {
            return true
        }
        return false
    }

    function isDesktop() {
        if (Qt.platform.os == "windows" ||
                Qt.platform.os == "linux" ||
                Qt.platform.os == "osx") {
            return true
        }
        return false
    }

    function trX(x) {
        return x
    }

    function trY(y) {
        return y
    }

    function trW(w) {
        return w
    }

    function trH(h) {
        return h
    }
}
