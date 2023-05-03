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

    function gotoPage(page, reset, slide) {
        if (pageStackView.depth > 0) {
            pageStackView.currentItem.deactivated()
        }

        if (slide) {
            pageStackView.push(UiLogic.pageEnumToString(page), {}, StackView.PushTransition)
        } else {
            pageStackView.push(UiLogic.pageEnumToString(page), {}, StackView.Immediate)
        }

        pageStackView.currentItem.activated(reset)
    }

    function closePage() {
        if (pageStackView.depth <= 1) {
            return
        }
        pageStackView.currentItem.deactivated()
        pageStackView.pop()
    }

    function setStartPage(page, slide) {
        if (pageStackView.depth > 0) {
            pageStackView.currentItem.deactivated()
        }

        pageStackView.clear()
        if (slide) {
            pageStackView.push(UiLogic.pageEnumToString(page), {}, StackView.PushTransition)
        } else {
            pageStackView.push(UiLogic.pageEnumToString(page), {}, StackView.Immediate)
        }
        if (page === PageEnum.Start) {
            UiLogic.pushButtonBackFromStartVisible = !pageStackView.empty
            UiLogic.onUpdatePage();
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "#0E0E11"
    }

    StackView {
        id: pageStackView
        anchors.fill: parent
        focus: true
    }

    Connections {
        target: UiLogic
        function onGoToPage(page, reset, slide) {
            root.gotoPage(page, reset, slide)
        }

        function onClosePage() {
            root.closePage()
        }

        function onSetStartPage(page, slide) {
            root.setStartPage(page, slide)
        }

        function onShow() {
            root.show()
            UiLogic.initializeUiLogic()
        }

        function onHide() {
            root.hide()
        }

        function onRaise() {
            root.show()
            root.raise()
            root.requestActivate()
        }
    }
}
