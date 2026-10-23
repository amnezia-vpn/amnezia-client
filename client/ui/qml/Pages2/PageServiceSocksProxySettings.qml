import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import PageEnum 1.0
import ProtocolEnum 1.0
import ContainerProps 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    Connections {
        target: InstallController

        function onUpdateContainerFinished() {
            PageController.showNotificationMessage(qsTr("Settings updated successfully"))
        }
    }

    ListViewType {
        id: listView

        anchors.top: parent.top
        anchors.bottom: changeSettingsButton.top
        anchors.bottomMargin: 16
        anchors.left: parent.left
        anchors.right: parent.right

        header: NavigationBarType {
            width: listView.width

            title: qsTr("SOCKS5 settings")
        }

        model: Socks5ProxyConfigModel

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            function openSettings() {
                changeSettingsDrawer.open()
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 16

                dense: true
                overline: qsTr("Host")
                text: ServersUiController.serverHostName(ServersUiController.processedServerId)
                Accessible.name: overline + " " + text

                trailingIconSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    GC.copyToClipBoard(text)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                dense: true
                overline: qsTr("Port")
                text: port
                Accessible.name: overline + " " + text

                trailingIconSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    GC.copyToClipBoard(text)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                dense: true
                overline: qsTr("User name")
                text: username
                Accessible.name: overline + " " + text

                trailingIconSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    GC.copyToClipBoard(text)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            ListItemType {
                id: passwordLabel

                property bool masked: true

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                showSeparator: false
                dense: true
                overline: qsTr("Password")
                text: masked ? "*".repeat(password.length) : password
                Accessible.name: overline

                trailingIconSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    GC.copyToClipBoard(password)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }

                IconButtonType {
                    icon.source: passwordLabel.masked ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
                    Accessible.name: passwordLabel.masked ? qsTr("Show password") : qsTr("Hide password")

                    onClicked: passwordLabel.masked = !passwordLabel.masked
                }
            }

            SheetType {
                id: changeSettingsDrawer

                parent: root

                expandedHeight: drawerContent.implicitHeight + 16 + PageController.safeAreaBottomMargin

                ColumnLayout {
                    id: drawerContent

                    property string tempPort: port
                    property string tempUsername: username
                    property string tempPassword: password

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16

                    spacing: 12

                    Connections {
                        target: changeSettingsDrawer

                        function onOpened() {
                            drawerContent.tempPort = port
                            drawerContent.tempUsername = username
                            drawerContent.tempPassword = password
                        }

                        function onClosed() {
                            port = drawerContent.tempPort
                            username = drawerContent.tempUsername
                            password = drawerContent.tempPassword
                            portTextField.textField.text = port
                            usernameTextField.textField.text = username
                            passwordTextField.textField.text = password
                        }
                    }

                    SectionHeaderType {
                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.bottomMargin: 12

                        headerText: qsTr("SOCKS5 settings")
                    }

                    TextFieldType {
                        id: portTextField

                        Layout.fillWidth: true

                        headerText: qsTr("Port")
                        textField.text: port
                        textField.maximumLength: 5
                        textField.validator: IntValidator { bottom: 1; top: 65535 }

                        textField.onEditingFinished: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            if (textField.text !== port) {
                                port = textField.text
                            }
                        }
                    }

                    TextFieldType {
                        id: usernameTextField

                        Layout.fillWidth: true

                        headerText: qsTr("Username")
                        textField.placeholderText: "username"
                        textField.text: username
                        textField.maximumLength: 32

                        textField.onEditingFinished: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            if (textField.text !== username) {
                                username = textField.text
                            }
                        }
                    }

                    TextFieldType {
                        id: passwordTextField

                        Layout.fillWidth: true

                        headerText: qsTr("Password")
                        textField.placeholderText: "password"
                        textField.text: password
                        textField.maximumLength: 32
                        passwordToggle: true

                        textField.onFocusChanged: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            if (textField.text !== password) {
                                password = textField.text
                            }
                        }
                    }

                    ButtonType {
                        id: saveButton

                        Layout.fillWidth: true
                        Layout.topMargin: 12

                        text: qsTr("Change connection settings")

                        onClicked: {
                            if (!portTextField.textField.acceptableInput) {
                                portTextField.errorText = qsTr("The port must be in the range of 1 to 65535")
                                return
                            }
                            if (usernameTextField.textField.text && passwordTextField.textField.text === "") {
                                passwordTextField.errorText = qsTr("Password cannot be empty")
                                return
                            } else if (usernameTextField.textField.text === "" && passwordTextField.textField.text) {
                                usernameTextField.errorText = qsTr("Username cannot be empty")
                                return
                            }

                            PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                            InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.Socks5Proxy)
                            drawerContent.tempPort = portTextField.textField.text
                            drawerContent.tempUsername = usernameTextField.textField.text
                            drawerContent.tempPassword = passwordTextField.textField.text
                            changeSettingsDrawer.close()
                        }
                    }
                }
            }
        }
    }

    ButtonType {
        id: changeSettingsButton

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        text: qsTr("Change connection settings")

        onClicked: {
            var content = listView.itemAtIndex(0)
            if (content) {
                content.openSettings()
            }
        }
    }
}
