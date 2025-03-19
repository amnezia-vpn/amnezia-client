import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import Config 1.0
import PageEnum 1.0

import "Controls"
import "Pages"

ApplicationWindow {
    id: root
    objectName: "mainWindow"
    visible: true
    width: DeviceInfo.screenWidth
    height: DeviceInfo.screenHeight
    minimumWidth: DeviceInfo.isDesktop() ? 360 : 0
    minimumHeight: DeviceInfo.isDesktop() ? 640 : 0
    maximumWidth: 600
    maximumHeight: 800

    color: Style.color.white

    onClosing: function() {
        console.debug("QML onClosing signal")
        PageController.closeWindow()
    }

    title: "DefaultVPN"

    Connections {
        target: PageController

        function onRaiseMainWindow() {
            root.show()
            root.raise()
            root.requestActivate()
        }

        function onHideMainWindow() {
            root.hide()
        }

        function onShowErrorMessage(errorMessage) {
            popupErrorMessage.text = errorMessage
            popupErrorMessage.open()
        }

        function onShowNotificationMessage(message) {
            popupNotificationMessage.text = message
            popupNotificationMessage.closeButtonVisible = false
            popupNotificationMessage.open()
            popupNotificationTimer.start()
        }

        function onShowBusyIndicator(visible) {
            busyIndicator.visible = visible
            PageController.disableControls(visible)
        }

        function onClosePage() {
            if (stackview.depth <= 1) {
                PageController.hideWindow()
                return
            }
            stackview.pop()
        }

        function onGoToPage(page, slide) {
            var pagePath = PageController.getPagePath(page)

            if (slide) {
                stackview.push(pagePath, { "objectName" : pagePath }, StackView.PushTransition)
            } else {
                stackview.push(pagePath, { "objectName" : pagePath }, StackView.Immediate)
            }
        }

        function onGoToStartPage() {
            while (stackview.depth > 1) {
                stackview.pop()
            }
        }
    }

    Connections {
        target: SettingsController

        function onChangeSettingsFinished(finishedMessage) {
            PageController.showNotificationMessage(finishedMessage)
        }
    }

    StackView {
        id: stackview
        anchors.fill: parent

        Component.onCompleted: {
            var pagePath = PageController.getPagePath(PageEnum.PageHome)
            ServersModel.processedIndex = ServersModel.defaultIndex

            stackview.push(pagePath, { "objectName" : pagePath })
        }
    }

    Item {
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        implicitHeight: popupNotificationMessage.height

        PopupType {
            id: popupNotificationMessage
        }

        Timer {
            id: popupNotificationTimer

            interval: 3000
            repeat: false
            running: false
            onTriggered: {
                popupNotificationMessage.close()
            }
        }
    }

    Item {
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        implicitHeight: popupErrorMessage.height

        PopupType {
            id: popupErrorMessage
        }
    }

    // Item {
    //     anchors.fill: parent

    //     QuestionDrawer {
    //         id: questionDrawer

    //         anchors.fill: parent
    //     }
    // }

    Item {
        anchors.fill: parent

        BusyIndicatorType {
            id: busyIndicator
            anchors.centerIn: parent
            z: 1
        }
    }

    // function showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction) {
    //     questionDrawer.headerText = headerText
    //     questionDrawer.descriptionText = descriptionText
    //     questionDrawer.yesButtonText = yesButtonText
    //     questionDrawer.noButtonText = noButtonText

    //     questionDrawer.yesButtonFunction = function() {
    //         questionDrawer.close()
    //         if (yesButtonFunction && typeof yesButtonFunction === "function") {
    //             yesButtonFunction()
    //         }
    //     }
    //     questionDrawer.noButtonFunction = function() {
    //         questionDrawer.close()
    //         if (noButtonFunction && typeof noButtonFunction === "function") {
    //             noButtonFunction()
    //         }
    //     }
    //     questionDrawer.open()
    // }

    FileDialog {
        id: mainFileDialog

        property bool isSaveMode: false

        objectName: "mainFileDialog"
        fileMode: isSaveMode ? FileDialog.SaveFile : FileDialog.OpenFile

        onAccepted: SystemController.fileDialogClosed(true)
        onRejected: SystemController.fileDialogClosed(false)
    }
}
