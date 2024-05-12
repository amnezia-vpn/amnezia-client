import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ContainerProps 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    defaultActiveFocusItem: listview

    Connections {
        target: InstallController

        function onUpdateContainerFinished() {
            PageController.showNotificationMessage(qsTr("Settings updated successfully"))
        }
    }

    Item {
        id: focusItem
        KeyNavigation.tab: backButton
    }

    ColumnLayout {
        id: backButtonLayout

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        anchors.topMargin: 20

        BackButtonType {
            id: backButton
            KeyNavigation.tab: listview
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
                        KeyNavigation.tab: portLabel.rightButton

                        text: qsTr("Host")
                        descriptionText: ServersModel.getProcessedServerData("hostName")

                        descriptionOnTop: true

                        rightImageSource: "qrc:/images/controls/copy.svg"
                        rightImageColor: "#D7D8DB"

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
                        KeyNavigation.tab: usernameLabel.rightButton

                        rightImageSource: "qrc:/images/controls/copy.svg"
                        rightImageColor: "#D7D8DB"

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
                        KeyNavigation.tab: passwordLabel.eyeButton

                        rightImageSource: "qrc:/images/controls/copy.svg"
                        rightImageColor: "#D7D8DB"

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
                        eyeButton.KeyNavigation.tab: passwordLabel.rightButton
                        rightButton.KeyNavigation.tab: removeButton

                        rightImageSource: "qrc:/images/controls/copy.svg"
                        rightImageColor: "#D7D8DB"

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

                        expandedContent: ColumnLayout {
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.topMargin: 32
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            spacing: 0

                            Connections {
                                target: changeSettingsDrawer
                                enabled: !GC.isMobile()
                                function onOpened() {
                                    drawerFocusItem.forceActiveFocus()
                                }
                            }

                            Item {
                                id: drawerFocusItem
                                KeyNavigation.tab: portTextField.textField
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

                                KeyNavigation.tab: usernameTextField.textField
                            }

                            TextFieldWithHeaderType {
                                id: usernameTextField

                                Layout.fillWidth: true
                                Layout.topMargin: 16
                                parentFlickable: fl

                                headerText: qsTr("Username")
                                textFieldPlaceholderText: "username"
                                textFieldText: username

                                textField.onEditingFinished: {
                                    textFieldText = textField.text.replace(/^\s+|\s+$/g, '')
                                    if (textFieldText !== username) {
                                        username = textFieldText
                                    }
                                }

                                KeyNavigation.tab: passwordTextField.textField
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

                                KeyNavigation.tab: saveButton
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

                                    PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                                    InstallController.updateContainer(Socks5ProxyConfigModel.getConfig())
                                    changeSettingsDrawer.close()
                                }
                            }
                        }
                    }

                    BasicButtonType {
                        id: removeButton
                        Layout.topMargin: 24
                        Layout.bottomMargin: 16
                        Layout.leftMargin: 8
                        implicitHeight: 32

                        defaultColor: "transparent"
                        hoveredColor: Qt.rgba(1, 1, 1, 0.08)
                        pressedColor: Qt.rgba(1, 1, 1, 0.12)
                        textColor: "#EB5757"

                        text: qsTr("Remove SOCKS5 proxy server")

                        KeyNavigation.tab: changeSettingsButton

                        clickedFunc: function() {
                            var headerText = qsTr("All users with whom you shared a server will no longer be able to connect to it.")
                            var yesButtonText = qsTr("Continue")
                            var noButtonText = qsTr("Cancel")

                            var yesButtonFunction = function() {
                                PageController.goToPage(PageEnum.PageDeinstalling)
                                InstallController.removeProcessedContainer()
                            }
                            var noButtonFunction = function() {
                                if (!GC.isMobile()) {
                                    removeButton.forceActiveFocus()
                                }
                            }

                            showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
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

//                            PageController.goToPage(PageEnum.PageSetupWizardInstalling)
//                            InstallController.updateContainer(Socks5ProxyConfigModel.getConfig())
                        }
                    }
                }
            }
        }
    }
}
