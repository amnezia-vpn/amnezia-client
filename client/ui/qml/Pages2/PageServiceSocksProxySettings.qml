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

    function openSettings() {
        changeSettingsDrawer.open()
    }

    FlickableType {
        id: flickable

        anchors.top: parent.top
        anchors.bottom: changeSettingsButton.top
        anchors.bottomMargin: 24

        contentHeight: content.implicitHeight

        ColumnLayout {
            id: content

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            spacing: 0

            NavigationBarType {
                Layout.fillWidth: true

                title: qsTr("SOCKS5 settings")
            }

            ListItemType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

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
                text: Socks5ProxyConfigModel.port
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
                text: Socks5ProxyConfigModel.username
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
                text: masked ? "*".repeat(Socks5ProxyConfigModel.password.length) : Socks5ProxyConfigModel.password
                Accessible.name: overline

                trailingIconSource: "qrc:/images/controls/copy.svg"

                onClicked: {
                    GC.copyToClipBoard(Socks5ProxyConfigModel.password)
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

                expandedHeight: drawerContent.implicitHeight + 16 + PageController.safeAreaBottomMargin + PageController.imeHeight

                ColumnLayout {
                    id: drawerContent

                    property string tempPort: Socks5ProxyConfigModel.port
                    property string tempUsername: Socks5ProxyConfigModel.username
                    property string tempPassword: Socks5ProxyConfigModel.password

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16

                    spacing: 12

                    Connections {
                        target: changeSettingsDrawer

                        function onOpened() {
                            drawerContent.tempPort = Socks5ProxyConfigModel.port
                            drawerContent.tempUsername = Socks5ProxyConfigModel.username
                            drawerContent.tempPassword = Socks5ProxyConfigModel.password
                        }

                        function onClosed() {
                            Socks5ProxyConfigModel.port = drawerContent.tempPort
                            Socks5ProxyConfigModel.username = drawerContent.tempUsername
                            Socks5ProxyConfigModel.password = drawerContent.tempPassword
                            portTextField.textField.text = Socks5ProxyConfigModel.port
                            usernameTextField.textField.text = Socks5ProxyConfigModel.username
                            passwordTextField.textField.text = Socks5ProxyConfigModel.password
                        }
                    }

                    SectionHeaderType {
                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.bottomMargin: 20

                        headerText: qsTr("SOCKS5 settings")
                    }

                    TextFieldType {
                        id: portTextField

                        Layout.fillWidth: true

                        headerText: qsTr("Port")
                        textField.text: Socks5ProxyConfigModel.port
                        textField.maximumLength: 5
                        textField.validator: IntValidator { bottom: 1; top: 65535 }

                        textField.onTextEdited: Socks5ProxyConfigModel.port = textField.text

                        textField.onEditingFinished: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            Socks5ProxyConfigModel.port = textField.text
                        }
                    }

                    TextFieldType {
                        id: usernameTextField

                        Layout.fillWidth: true

                        headerText: qsTr("Username")
                        textField.placeholderText: "username"
                        textField.text: Socks5ProxyConfigModel.username
                        textField.maximumLength: 32

                        textField.onTextEdited: Socks5ProxyConfigModel.username = textField.text

                        textField.onEditingFinished: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            Socks5ProxyConfigModel.username = textField.text
                        }
                    }

                    TextFieldType {
                        id: passwordTextField

                        Layout.fillWidth: true

                        headerText: qsTr("Password")
                        textField.placeholderText: "password"
                        textField.text: Socks5ProxyConfigModel.password
                        textField.maximumLength: 32
                        passwordToggle: true

                        textField.onTextEdited: Socks5ProxyConfigModel.password = textField.text

                        textField.onEditingFinished: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            Socks5ProxyConfigModel.password = textField.text
                        }
                    }

                    ButtonType {
                        id: saveButton

                        Layout.fillWidth: true
                        Layout.topMargin: 12

                        text: qsTr("Change connection settings")

                        onClicked: {
                            Socks5ProxyConfigModel.port = Socks5ProxyConfigModel.port.trim()
                            Socks5ProxyConfigModel.username = Socks5ProxyConfigModel.username.trim()
                            Socks5ProxyConfigModel.password = Socks5ProxyConfigModel.password.trim()

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
            root.openSettings()
        }
    }
}
