import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0

import "Config"
import "Controls2"
import "Components"
import "Pages2"

Window  {
    id: root
    objectName: "mainWindow"
    visible: true
    width: GC.screenWidth
    height: GC.screenHeight
    minimumWidth: GC.isDesktop() ? 360 : 0
    minimumHeight: GC.isDesktop() ? 640 : 0
    maximumWidth: 600
    maximumHeight: 800

    color: AmneziaStyle.color.midnightBlack

    onClosing: function() {
        console.debug("QML onClosing signal")
        PageController.closeWindow()
    }

    title: "AmneziaVPN"

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

        function onShowPassphraseRequestDrawer() {
            privateKeyPassphraseDrawer.open()
        }

        function onGoToPageSettingsBackup() {
            PageController.goToPage(PageEnum.PageSettingsBackup)
        }

        function onShowBusyIndicator(visible) {
            busyIndicator.visible = visible
            PageController.disableControls(visible)
        }
    }

    Connections {
        target: SettingsController

        function onChangeSettingsFinished(finishedMessage) {
            PageController.showNotificationMessage(finishedMessage)
        }
    }

    PageStart {
        anchors.fill: parent
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

    Item {
        anchors.fill: parent

        DrawerType2 {
            id: privateKeyPassphraseDrawer

            anchors.fill: parent
            expandedHeight: root.height * 0.35

            expandedContent: ColumnLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16
                anchors.leftMargin: 16
                anchors.rightMargin: 16

                Connections {
                    target: privateKeyPassphraseDrawer
                    function onOpened() {
                        passphrase.textFieldText = ""
                        passphrase.textField.forceActiveFocus()
                    }

                    function onAboutToHide() {
                        if (passphrase.textFieldText !== "") {
                            PageController.showBusyIndicator(true)
                        }
                    }

                    function onAboutToShow() {
                        PageController.showBusyIndicator(false)
                    }
                }

                TextFieldWithHeaderType {
                    id: passphrase

                    property bool hidePassword: true

                    Layout.fillWidth: true
                    headerText: qsTr("Private key passphrase")
                    textField.echoMode: hidePassword ? TextInput.Password : TextInput.Normal
                    buttonImageSource: hidePassword ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"

                    clickedFunc: function() {
                        hidePassword = !hidePassword
                    }

                    KeyNavigation.tab: saveButton
                }

                BasicButtonType {
                    id: saveButton

                    Layout.fillWidth: true

                    defaultColor: AmneziaStyle.color.transparent
                    hoveredColor: AmneziaStyle.color.translucentWhite
                    pressedColor: AmneziaStyle.color.sheerWhite
                    disabledColor: AmneziaStyle.color.mutedGray
                    textColor: AmneziaStyle.color.paleGray
                    borderWidth: 1

                    text: qsTr("Save")

                    clickedFunc: function() {
                        privateKeyPassphraseDrawer.close()
                        PageController.passphraseRequestDrawerClosed(passphrase.textFieldText)
                    }
                }
            }
        }
    }

    Item {
        anchors.fill: parent

        QuestionDrawer {
            id: questionDrawer

            anchors.fill: parent
        }
    }

    Item {
        anchors.fill: parent

        BusyIndicatorType {
            id: busyIndicator
            anchors.centerIn: parent
            z: 1
        }
    }

    function showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction) {
        questionDrawer.headerText = headerText
        questionDrawer.descriptionText = descriptionText
        questionDrawer.yesButtonText = yesButtonText
        questionDrawer.noButtonText = noButtonText

        questionDrawer.yesButtonFunction = function() {
            questionDrawer.close()
            if (yesButtonFunction && typeof yesButtonFunction === "function") {
                yesButtonFunction()
            }
        }
        questionDrawer.noButtonFunction = function() {
            questionDrawer.close()
            if (noButtonFunction && typeof noButtonFunction === "function") {
                noButtonFunction()
            }
        }
        questionDrawer.open()
    }

    FileDialog {
        id: mainFileDialog

        property bool isSaveMode: false

        objectName: "mainFileDialog"
        fileMode: isSaveMode ? FileDialog.SaveFile : FileDialog.OpenFile

        onAccepted: SystemController.fileDialogClosed(true)
        onRejected: SystemController.fileDialogClosed(false)
    }
}
