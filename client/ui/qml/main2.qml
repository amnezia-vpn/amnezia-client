import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import PageType 1.0

import "Config"

Window  {
    id: root
    visible: true
    width: GC.screenWidth
    height: GC.screenHeight
    minimumWidth: GC.isDesktop() ? 360 : 0
    minimumHeight: GC.isDesktop() ? 640 : 0

    onClosing: function() {
        console.debug("QML onClosing signal")
        UiLogic.onCloseWindow()
    }

    title: "AmneziaVPN"

    Rectangle {
        anchors.fill: parent
        color: "#0E0E11"
    }

    StackView {
        anchors.fill: parent
        focus: true
        initialItem: PageController.getInitialPage()
    }
}
