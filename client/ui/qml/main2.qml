import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import PageEnum 1.0

import "Config"
import "Controls2"

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

    color: "#0E0E11"

    onClosing: function() {
        console.debug("QML onClosing signal")
        PageController.closeWindow()
    }

    title: "AmneziaVPN"

    StackViewType {
        id: rootStackView

        anchors.fill: parent
        focus: true

        Component.onCompleted: {
            var pagePath = PageController.getInitialPage()
            rootStackView.push(pagePath, { "objectName" : pagePath })
        }

        Keys.onPressed: function(event) {
            PageController.keyPressEvent(event.key)
            event.accepted = true
        }
    }

    Connections {
        target: PageController

        function onReplaceStartPage() {
            var pagePath = PageController.getInitialPage()
            rootStackView.clear()
            PageController.updateNavigationBarColor(PageController.getInitialPageNavigationBarColor())
            rootStackView.replace(pagePath, { "objectName" : pagePath })
        }

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

        function onSetupFileDialogForConfig() {
            mainFileDialog.acceptLabel = qsTr("Open config file")
            mainFileDialog.nameFilters = !ServersModel.getServersCount() ? [ "Config or backup files (*.vpn *.ovpn *.conf *.backup)" ] :
                                                                        [ "Config files (*.vpn *.ovpn *.conf)" ]
            mainFileDialog.acceptFunction = function() {
                if (mainFileDialog.selectedFile.toString().indexOf(".backup") !== -1 && !ServersModel.getServersCount()) {
                    PageController.showBusyIndicator(true)
                    SettingsController.restoreAppConfig(mainFileDialog.selectedFile.toString())
                    PageController.showBusyIndicator(false)
                } else {
                    ImportController.extractConfigFromFile(mainFileDialog.selectedFile)
                    PageController.goToPage(PageEnum.PageSetupWizardViewConfig)
                }
            }
        }

        function onSetupFileDialogForSites(replaceExistingSites) {
            mainFileDialog.acceptLabel = qsTr("Open sites file")
            mainFileDialog.nameFilters = [ "Sites files (*.json)" ]
            mainFileDialog.acceptFunction = function() {
                PageController.showBusyIndicator(true)
                SitesController.importSites(mainFileDialog.selectedFile.toString(), replaceExistingSites)
                PageController.showBusyIndicator(false)
            }
        }

        function onSetupFileDialogForBackup() {
            mainFileDialog.acceptLabel = qsTr("Open backup file")
            mainFileDialog.nameFilters = [ "Backup files (*.backup)" ]
            mainFileDialog.acceptFunction = function() {
                PageController.showBusyIndicator(true)
                SettingsController.restoreAppConfig(mainFileDialog.selectedFile.toString())
                PageController.showBusyIndicator(false)
            }
        }
    }

    Connections {
        target: SettingsController
        function onChangeSettingsFinished(finishedMessage) {
            PageController.showNotificationMessage(finishedMessage)
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

    Item {
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom

        implicitHeight: popupErrorMessage.height

        DrawerType {
            id: privateKeyPassphraseDrawer

            width: root.width
            height: root.height * 0.35

            onVisibleChanged: {
                if (privateKeyPassphraseDrawer.visible) {
                    passphrase.textFieldText = ""
                    passphrase.textField.forceActiveFocus()
                }
            }
            onAboutToHide: {
                PageController.showBusyIndicator(true)
            }
            onAboutToShow: {
                PageController.showBusyIndicator(false)
            }

            ColumnLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 16
                anchors.leftMargin: 16
                anchors.rightMargin: 16

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
                }

                BasicButtonType {
                    Layout.fillWidth: true

                    defaultColor: "transparent"
                    hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                    pressedColor: Qt.rgba(1, 1, 1, 0.12)
                    disabledColor: "#878B91"
                    textColor: "#D7D8DB"
                    borderWidth: 1

                    text: qsTr("Save")

                    onClicked: {
                        privateKeyPassphraseDrawer.close()
                        PageController.passphraseRequestDrawerClosed(passphrase.textFieldText)
                    }
                }
            }
        }
    }

    FileDialog {
        id: mainFileDialog

        property var acceptFunction

        objectName: "mainFileDialog"

        onAccepted: acceptFunction()
    }
}
