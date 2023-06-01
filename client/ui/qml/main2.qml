import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

import PageType 1.0
import PageEnum 1.0

import "Config"
import "Controls2"

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

    StackViewType {
        id: rootStackView

        anchors.fill: parent
        focus: true

        Component.onCompleted: {
            var pagePath = PageController.getPagePath(PageEnum.PageStart)
            rootStackView.push(pagePath, { "objectName" : pagePath })
        }
    }

    Connections {
        target: PageController

        function onReplaceStartPage() {
            var pagePath = PageController.getInitialPage()
            while (rootStackView.depth > 1) {
                rootStackView.pop()
            }
            rootStackView.replace(pagePath, { "objectName" : pagePath })
        }
    }
}
