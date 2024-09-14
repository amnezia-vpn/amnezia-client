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

    ColumnLayout {
        id: backButtonLayout

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
        }
    }

    FlickableType {
        id: fl
        anchors.top: backButtonLayout.bottom
        anchors.bottom: parent.bottom
        contentHeight: listview.implicitHeight

        ListView {
            id: listview

            width: parent.width
            height: listview.contentItem.height

            clip: true
            interactive: false

            model: Socks5ProxyConfigModel

            onFocusChanged: {
                if (focus) {
                    listview.currentItem.focusItemId.forceActiveFocus()
                }
            }

            delegate: Item {
                implicitWidth: listview.width
                implicitHeight: content.implicitHeight

                property alias focusItemId: hostLabel.rightButton

                ColumnLayout {
                    id: content

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right

                    spacing: 0

                    HeaderType {
                        Layout.fillWidth: true
                        Layout.leftMargin: 16
                        Layout.rightMargin: 16

                        headerText: qsTr("SOCKS5 settings")
                    }

                    LabelWithButtonType {
                        id: hostLabel
                        Layout.fillWidth: true
                        Layout.topMargin: 32

                        parentFlickable: fl

                        text: qsTr("Host")
                        descriptionText: ServersModel.getProcessedServerData("hostName")

                        descriptionOnTop: true

                        rightImageSource: "qrc:/images/controls/copy.svg"
                        rightImageColor: AmneziaStyle.color.paleGray

                        clickedFunction: function() {
                            GC.copyToClipBoard(descriptionText)
                            PageController.showNotificationMessage(qsTr("Copied"))
                            if (!GC.isMobile()) {
                                this.rightButton.forceActiveFocus()
                            }
                        }
                    }

                    LabelWithButtonType {
                        id: portLabel
                        Layout.fillWidth: true

                        text: qsTr("Port")
                        descriptionText: port

                        descriptionOnTop: true

                        parentFlickable: fl

                        rightImageSource: "qrc:/images/controls/copy.svg"
                        rightImageColor: AmneziaStyle.color.paleGray

                        clickedFunction: function() {
                            GC.copyToClipBoard(descriptionText)
                            PageController.showNotificationMessage(qsTr("Copied"))
                            if (!GC.isMobile()) {
                                this.rightButton.forceActiveFocus()
                            }
                        }
                    }

                    LabelWithButtonType {
                        id: usernameLabel
                        Layout.fillWidth: true

                        text: qsTr("User name")
                        descriptionText: username

                        descriptionOnTop: true

                        parentFlickable: fl

                        rightImageSource: "qrc:/images/controls/copy.svg"
                        rightImageColor: AmneziaStyle.color.paleGray

                        clickedFunction: function() {
                            GC.copyToClipBoard(descriptionText)
                            PageController.showNotificationMessage(qsTr("Copied"))
                            if (!GC.isMobile()) {
                                this.rightButton.forceActiveFocus()
                            }
                        }
                    }

                    LabelWithButtonType {
                        id: passwordLabel
                        Layout.fillWidth: true

                        text: qsTr("Password")
                        descriptionText: password

                        descriptionOnTop: true

                        parentFlickable: fl

                        rightImageSource: "qrc:/images/controls/copy.svg"
                        rightImageColor: AmneziaStyle.color.paleGray

                        buttonImageSource: hideDescription ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"

                        clickedFunction: function() {
                            GC.copyToClipBoard(descriptionText)
                            PageController.showNotificationMessage(qsTr("Copied"))
                            if (!GC.isMobile()) {
                                this.rightButton.forceActiveFocus()
                            }
                        }
                    }

                    DrawerType2 {
                        id: changeSettingsDrawer
                        parent: root

                        anchors.fill: parent
                        expandedHeight: root.height * 0.9

                        onClosed: {
                            if (!GC.isMobile()) {
                                focusItem.forceActiveFocus()
                            }
                        }

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
                                    if (!GC.isMobile()) {
                                        drawerFocusItem.forceActiveFocus()
                                    }
                                    tempPort = port
                                    tempUsername = username
                                    tempPassword = password
                                }
                                function onClosed() {
                                    port = tempPort
                                    username = tempUsername
                                    password = tempPassword
                                    portTextField.textFieldText = port
                                    usernameTextField.textFieldText = username
                                    passwordTextField.textFieldText = password
                                }
                            }

                            HeaderType {
                                Layout.fillWidth: true

                                headerText: qsTr("SOCKS5 settings")
                            }

                            TextFieldWithHeaderType {
                                id: portTextField

                                Layout.fillWidth: true
                                Layout.topMargin: 40
                                parentFlickable: fl

                                headerText: qsTr("Port")
                                textFieldText: port
                                textField.maximumLength: 5
                                textField.validator: IntValidator { bottom: 1; top: 65535 }

                                textField.onEditingFinished: {
                                    textFieldText = textField.text.replace(/^\s+|\s+$/g, '')
                                    if (textFieldText !== port) {
                                        port = textFieldText
                                    }
                                }
                            }

                            TextFieldWithHeaderType {
                                id: usernameTextField

                                Layout.fillWidth: true
                                Layout.topMargin: 16
                                parentFlickable: fl

                                headerText: qsTr("Username")
                                textFieldPlaceholderText: "username"
                                textFieldText: username
                                textField.maximumLength: 32

                                textField.onEditingFinished: {
                                    textFieldText = textField.text.replace(/^\s+|\s+$/g, '')
                                    if (textFieldText !== username) {
                                        username = textFieldText
                                    }
                                }
                            }

                            TextFieldWithHeaderType {
                                id: passwordTextField

                                property bool hidePassword: true

                                Layout.fillWidth: true
                                Layout.topMargin: 16
                                parentFlickable: fl

                                headerText: qsTr("Password")
                                textFieldPlaceholderText: "password"
                                textFieldText: password
                                textField.maximumLength: 32

                                textField.echoMode: hidePassword ? TextInput.Password : TextInput.Normal
                                buttonImageSource: textFieldText !== "" ? (hidePassword ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg")
                                                                        : ""

                                clickedFunc: function() {
                                    hidePassword = !hidePassword
                                }

                                textField.onFocusChanged: {
                                    textFieldText = textField.text.replace(/^\s+|\s+$/g, '')
                                    if (textFieldText !== password) {
                                        password = textFieldText
                                    }
                                }
                            }

                            BasicButtonType {
                                id: saveButton

                                Layout.fillWidth: true
                                Layout.topMargin: 24
                                Layout.bottomMargin: 24

                                text: qsTr("Change connection settings")
                                Keys.onTabPressed: lastItemTabClicked(drawerFocusItem)

                                clickedFunc: function() {
                                    forceActiveFocus()

                                    if (!portTextField.textField.acceptableInput) {
                                        portTextField.errorText = qsTr("The port must be in the range of 1 to 65535")
                                        return
                                    }
                                    if (usernameTextField.textFieldText && passwordTextField.textFieldText === "") {
                                        passwordTextField.errorText = qsTr("Password cannot be empty")
                                        return
                                    } else if (usernameTextField.textFieldText === "" && passwordTextField.textFieldText) {
                                        usernameTextField.errorText = qsTr("Username cannot be empty")
                                        return
                                    }

                                    PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                                    InstallController.updateContainer(Socks5ProxyConfigModel.getConfig())
                                    tempPort = portTextField.textFieldText
                                    tempUsername = usernameTextField.textFieldText
                                    tempPassword = passwordTextField.textFieldText
                                    changeSettingsDrawer.close()
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
                        Keys.onTabPressed: lastItemTabClicked(focusItem)

                        clickedFunc: function() {
                            forceActiveFocus()
                            changeSettingsDrawer.open()
                        }
                    }
                }
            }
        }
    }
}
