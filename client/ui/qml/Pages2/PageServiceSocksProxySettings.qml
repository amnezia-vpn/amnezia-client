import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
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

    BackButtonType {
        id: backButton

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.topMargin: 20

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

        model: Socks5ProxyConfigModel

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("SOCKS5 settings")
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 32
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                text: qsTr("Host")
                descriptionText: ServersModel.getProcessedServerData("hostName")

                descriptionOnTop: true

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                text: qsTr("Port")
                descriptionText: port

                descriptionOnTop: true

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                text: qsTr("User name")
                descriptionText: username

                descriptionOnTop: true

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                text: qsTr("Password")
                descriptionText: password

                descriptionOnTop: true

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                buttonImageSource: hideDescription ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"

                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            DrawerType2 {
                id: changeSettingsDrawer
                parent: root

                anchors.fill: parent
                expandedHeight: root.height * 0.9

                expandedStateContent: ColumnLayout {
                    property string tempPort: port
                    property string tempUsername: username
                    property string tempPassword: password

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.topMargin: 32
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 0

                    Connections {
                        target: changeSettingsDrawer
                        function onOpened() {
                            tempPort = port
                            tempUsername = username
                            tempPassword = password
                        }
                        function onClosed() {
                            port = tempPort
                            username = tempUsername
                            password = tempPassword
                            portTextField.textField.text = port
                            usernameTextField.textField.text = username
                            passwordTextField.textField.text = password
                        }
                    }

                    BaseHeaderType {
                        Layout.fillWidth: true
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16

                        headerText: qsTr("SOCKS5 settings")
                    }

                    TextFieldWithHeaderType {
                        id: portTextField

                        Layout.fillWidth: true
                        Layout.topMargin: 40
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16

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

                    TextFieldWithHeaderType {
                        id: usernameTextField

                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16

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

                    TextFieldWithHeaderType {
                        id: passwordTextField

                        property bool hidePassword: true

                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16

                        headerText: qsTr("Password")
                        textField.placeholderText: "password"
                        textField.text: password
                        textField.maximumLength: 32

                        textField.echoMode: hidePassword ? TextInput.Password : TextInput.Normal
                        buttonImageSource: textField.text !== "" ? (hidePassword ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg")
                                                                 : ""

                        clickedFunc: function() {
                            hidePassword = !hidePassword
                        }

                        textField.onFocusChanged: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            if (textField.text !== password) {
                                password = textField.text
                            }
                        }
                    }

                    BasicButtonType {
                        id: saveButton

                        Layout.fillWidth: true
                        Layout.topMargin: 24
                        Layout.bottomMargin: 24
                        Layout.rightMargin: 16

                        text: qsTr("Change connection settings")

                        clickedFunc: function() {
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
                            InstallController.updateContainer(Socks5ProxyConfigModel.getConfig())
                            tempPort = portTextField.textField.text
                            tempUsername = usernameTextField.textField.text
                            tempPassword = passwordTextField.textField.text
                            changeSettingsDrawer.closeTriggered()
                        }
                    }
                }
            }

            BasicButtonType {
                id: changeSettingsButton

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Change connection settings")

                clickedFunc: function() {
                    changeSettingsDrawer.openTriggered()
                }
            }
        }
    }
}
