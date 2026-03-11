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
        anchors.topMargin: 20 + SettingsController.safeAreaTopMargin
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

        enabled: ServersModel.isProcessedServerHasWriteAccess()
        model: MtproxyConfigModel

        delegate: ColumnLayout {
            width: listView.width
            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                headerText: qsTr("MTProxy settings")
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
                text: qsTr("Secret")
                descriptionText: secret
                descriptionOnTop: true
                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray
                buttonImageSource: hideDescription ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg"
                clickedFunction: function() {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.bottomMargin: 16
                text: qsTr("Tag")
                descriptionText: tag
                descriptionOnTop: true
                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray
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
                    property string tempSecret: secret
                    property string tempTag: tag

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
                            tempSecret = secret
                            tempTag = tag
                        }
                        function onClosed() {
                            port = tempPort
                            secret = tempSecret
                            tag = tempTag
                            portTextField.textField.text = port
                            secretTextField.textField.text = secret
                            tagTextField.textField.text = tag
                        }
                    }

                    BaseHeaderType {
                        Layout.fillWidth: true
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        headerText: qsTr("MTProxy settings")
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
                        id: secretTextField
                        property bool hideSecret: true
                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        headerText: qsTr("Secret (32 hex)")
                        textField.placeholderText: qsTr("Leave empty to auto-generate")
                        textField.text: secret
                        textField.maximumLength: 32
                        textField.validator: RegularExpressionValidator {
                            regularExpression: /^[0-9a-fA-F]{0,32}$/
                        }
                        textField.echoMode: hideSecret ? TextInput.Password : TextInput.Normal
                        buttonImageSource: textField.text !== ""
                            ? (hideSecret ? "qrc:/images/controls/eye.svg" : "qrc:/images/controls/eye-off.svg")
                            : ""
                        clickedFunc: function() {
                            hideSecret = !hideSecret
                        }
                        textField.onEditingFinished: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            if (textField.text !== secret) {
                                secret = textField.text
                            }
                        }
                    }

                    TextFieldWithHeaderType {
                        id: tagTextField
                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16
                        headerText: qsTr("Tag (optional, 32 hex)")
                        textField.placeholderText: qsTr("Optional")
                        textField.text: tag
                        textField.maximumLength: 32
                        textField.validator: RegularExpressionValidator {
                            regularExpression: /^[0-9a-fA-F]{0,32}$/
                        }
                        textField.onEditingFinished: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            if (textField.text !== tag) {
                                tag = textField.text
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
                            if (!secretTextField.textField.acceptableInput) {
                                secretTextField.errorText = qsTr("Secret must be a 32-character hex string")
                                return
                            }
                            if (!tagTextField.textField.acceptableInput) {
                                tagTextField.errorText = qsTr("Tag must be a 32-character hex string")
                                return
                            }
                            if (secretTextField.textField.text !== "" && secretTextField.textField.text.length !== 32) {
                                secretTextField.errorText = qsTr("Secret must be a 32-character hex string")
                                return
                            }
                            if (tagTextField.textField.text !== "" && tagTextField.textField.text.length !== 32) {
                                tagTextField.errorText = qsTr("Tag must be a 32-character hex string")
                                return
                            }
                            PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                            InstallController.updateContainer(MtproxyConfigModel.getConfig())
                            tempPort = portTextField.textField.text
                            tempSecret = secretTextField.textField.text
                            tempTag = tagTextField.textField.text
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

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                text: qsTr("Remove ") + ContainersModel.getProcessedContainerName()
                textColor: AmneziaStyle.color.vibrantRed

                clickedFunction: function() {
                    var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        InstallController.removeProcessedContainer()
                    }
                    var noButtonFunction = function() {}

                    showQuestionDrawer(headerText, "", yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }
        }
    }
}
