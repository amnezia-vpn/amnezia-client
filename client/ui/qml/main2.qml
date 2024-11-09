import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0
import Style 1.0
import ScreenMarginInfo 1.0

import "Config"
import "Controls2"
import "Controls2/TextTypes"
import "Components"
import "Pages2"

ApplicationWindow  {
    id: root
    objectName: "mainWindow"
    visible: true
    width: GC.screenWidth
    height: GC.screenHeight
    minimumWidth: GC.isDesktop() ? 360 : 0
    minimumHeight: GC.isDesktop() ? 640 : 0
    maximumWidth: 600
    maximumHeight: 800

    flags: (Qt.platform.os === "ios" ? Qt.MaximizeUsingFullscreenGeometryHint : Qt.Window)

    color: AmneziaStyle.color.midnightBlack

    onClosing: function() {
        console.debug("QML onClosing signal")
        PageController.closeWindow()
    }

    title: "ZloVPN"

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

        function onShowSplitTunnelingFailed() {
            splitTunnelingFailed.open()
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
        width: root.width
        height: root.height
    }

    Item {
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        anchors.bottomMargin: ScreenMargins.margins.bottom
        anchors.leftMargin: ScreenMargins.margins.left
        anchors.rightMargin: ScreenMargins.margins.right

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

        anchors.bottomMargin: ScreenMargins.margins.bottom
        anchors.leftMargin: ScreenMargins.margins.left
        anchors.rightMargin: ScreenMargins.margins.right

        implicitHeight: popupErrorMessage.height

        PopupType {
            id: popupErrorMessage
        }
    }

    Popup {
        id: splitTunnelingFailed
        anchors.centerIn: Overlay.overlay
        background: Rectangle {
            radius: 16
            color: Qt.rgba(14/255, 14/255, 17/255, 1.0)
            border.color: "transparent"
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 100 }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 100 }
        }

        modal: true
        focus: true
        closePolicy: Popup.NoAutoClose

        padding: 20
        property int margin: 32
        property int maxWidth: 380
        width: Math.min(parent.width - margin, maxWidth)

        ColumnLayout {
            id: popupContent
            width: parent.width
            spacing: 16

            Header2Type {
                Layout.maximumWidth: parent.width

                headerText: qsTr("Split tunneling failed")
            }

            ParagraphTextType {
                Layout.fillWidth: true
                Layout.maximumWidth: parent.width

                font.pixelSize: 20

                text: qsTr("Possible reasons:")
                color: AmneziaStyle.color.paleGray
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.maximumWidth: parent.width

                ParagraphTextType {
                    Layout.alignment: Qt.AlignLeft
                    Layout.maximumWidth: parent.width
                    text: qsTr("• You have Mullvad VPN running")
                }
                ParagraphTextType {
                    Layout.alignment: Qt.AlignLeft
                    Layout.maximumWidth: parent.width
                    text: qsTr("• Split tunneling driver failed to start")
                }
                ParagraphTextType {
                    Layout.alignment: Qt.AlignLeft
                    Layout.maximumWidth: parent.width
                    text: qsTr("• Failed to set up firewall")
                }
            }

            BasicButtonType {
                id: goBackButton

                Layout.fillWidth: true

                text: qsTr("Go back")

                clickedFunc: function() {
                    splitTunnelingFailed.close()
                }
            }
        }
    }

    Item {
        anchors.fill: parent
        anchors.topMargin: ScreenMargins.margins.top
        anchors.bottomMargin: ScreenMargins.margins.bottom
        anchors.leftMargin: ScreenMargins.margins.left
        anchors.rightMargin: ScreenMargins.margins.right

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
