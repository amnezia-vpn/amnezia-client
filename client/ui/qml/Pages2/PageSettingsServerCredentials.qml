import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Config"
import "../Controls2/TextTypes"
import "../Components"

PageType {
    id: root

    readonly property string serverId: ServersUiController.processedServerId

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20 + PageController.safeAreaTopMargin

        onFocusChanged: {
            if (this.activeFocus) {
                listView.positionViewAtBeginning()
            }
        }
    }

    ListViewType {
        id: listView

        anchors.top: backButton.bottom
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.left: parent.left

        header: ColumnLayout {
            width: listView.width
            spacing: 16

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("SSH connection")
                descriptionText: qsTr("Used to manage the server: installing protocols and services, rebooting and clearing it. The VPN connection itself does not use them.")
            }
        }

        model: 1 // fake model to force the ListView to be created without a model

        delegate: ColumnLayout {
            width: listView.width
            spacing: 16

            LabelWithButtonType {
                Layout.fillWidth: true

                text: qsTr("Server address")
                descriptionText: ServersUiController.serverHostName(root.serverId)
                descriptionOnTop: true
            }

            DividerType {}

            TextFieldWithHeaderType {
                id: userNameField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("User name")
                textField.text: ServersUiController.serverUserName(root.serverId)
            }

            TextFieldWithHeaderType {
                id: portField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("SSH port")
                textField.text: ServersUiController.serverSshPort(root.serverId)
                textField.validator: IntValidator { bottom: 1; top: 65535 }
            }

            TextFieldWithHeaderType {
                id: passwordField

                property bool hidePassword: true

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("New password or private key")
                // The stored secret is never shown, it can only be replaced
                textField.placeholderText: qsTr("Leave empty to keep the current one")

                textField.echoMode: hidePassword ? TextInput.Password : TextInput.Normal
                buttonImageSource: textField.text !== "" ? (hidePassword ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg")
                                                         : ""

                clickedFunc: function() {
                    hidePassword = !hidePassword
                }
            }

            WarningType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                textString: qsTr("The address cannot be changed here: it is also the address your VPN configs point at, so a server that moved has to be added again. Everything else is tried against the server before it is saved, and nothing is stored if it does not work.")
                iconPath: "qrc:/images/controls/alert-circle.svg"
            }

            BasicButtonType {
                id: saveButton

                Layout.fillWidth: true
                Layout.margins: 16

                text: qsTr("Save")

                clickedFunc: function() {
                    var userName = userNameField.textField.text.replace(/^\s+|\s+$/g, '')
                    var port = parseInt(portField.textField.text)
                    var password = passwordField.textField.text.replace(/^\s+|\s+$/g, '')

                    userNameField.errorText = ""
                    portField.errorText = ""

                    if (userName === "") {
                        userNameField.errorText = qsTr("User name cannot be empty")
                        return
                    }
                    if (isNaN(port) || port < 1 || port > 65535) {
                        portField.errorText = qsTr("Port must be between 1 and 65535")
                        return
                    }

                    // Try them against the server first, the same check the setup
                    // wizard runs. On failure the controller reports the error and
                    // we stay on the page with the values still filled in.
                    PageController.showBusyIndicator(true)
                    var isConnectionOpened = InstallController.checkServerCredentials(root.serverId, userName, port, password)
                    PageController.showBusyIndicator(false)
                    if (!isConnectionOpened) {
                        return
                    }

                    ServersUiController.editServerCredentials(root.serverId, userName, port, password)
                    passwordField.textField.text = ""
                    PageController.closePage()
                }
            }
        }
    }
}
