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

        model: MtProxyConfigModel

        delegate: ColumnLayout {
            width: listView.width

            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("MTProxy settings")
                descriptionText: qsTr("Telegram MTProto proxy. Share the link below with users to connect through your server.")
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

                clickedFunction: function () {
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

                clickedFunction: function () {
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

                buttonImageSource: hideDescription
                    ? "qrc:/images/controls/eye.svg"
                    : "qrc:/images/controls/eye-off.svg"

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function () {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                visible: tag !== ""

                text: qsTr("Tag")
                descriptionText: tag
                descriptionOnTop: true

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function () {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                visible: tgLink !== ""

                text: qsTr("Telegram link (tg://)")
                descriptionText: tgLink
                descriptionOnTop: true
                textColor: AmneziaStyle.color.goldenApricot

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function () {
                    GC.copyToClipBoard(descriptionText)
                    PageController.showNotificationMessage(qsTr("Copied"))
                }
            }

            LabelWithButtonType {
                Layout.fillWidth: true
                Layout.rightMargin: 16
                Layout.bottomMargin: 16

                visible: tmeLink !== ""

                text: qsTr("Telegram link (t.me)")
                descriptionText: tmeLink
                descriptionOnTop: true
                textColor: AmneziaStyle.color.goldenApricot

                rightImageSource: "qrc:/images/controls/copy.svg"
                rightImageColor: AmneziaStyle.color.paleGray

                clickedFunction: function () {
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
                            tempTag = tag
                        }

                        function onClosed() {
                            port = tempPort
                            tag = tempTag
                            portTextField.textField.text = port
                            tagTextField.textField.text = tag
                        }
                    }

                    BaseHeaderType {
                        Layout.fillWidth: true
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16

                        headerText: qsTr("MTProxy settings")
                    }

                    // Port field
                    TextFieldWithHeaderType {
                        id: portTextField

                        Layout.fillWidth: true
                        Layout.topMargin: 40
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16

                        headerText: qsTr("Port")
                        textField.text: port
                        textField.maximumLength: 5
                        textField.validator: IntValidator {
                            bottom: 1; top: 65535
                        }

                        textField.onEditingFinished: {
                            textField.text = textField.text.replace(/^\s+|\s+$/g, '')
                            if (textField.text !== port) {
                                port = textField.text
                            }
                        }
                    }

                    TextFieldWithHeaderType {
                        id: tagTextField

                        Layout.fillWidth: true
                        Layout.topMargin: 16
                        Layout.rightMargin: 16
                        Layout.bottomMargin: 16

                        headerText: qsTr("Tag (optional, from @MTProxybot)")
                        textField.placeholderText: qsTr("leave empty if not needed")
                        textField.text: tag
                        textField.maximumLength: 64

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

                        clickedFunc: function () {
                            if (!portTextField.textField.acceptableInput) {
                                portTextField.errorText = qsTr("The port must be in the range of 1 to 65535")
                                return
                            }

                            PageController.goToPage(PageEnum.PageSetupWizardInstalling)
                            InstallController.updateContainer(MtProxyConfigModel.getConfig())
                            tempPort = portTextField.textField.text
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
                Layout.bottomMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: ServersModel.isProcessedServerHasWriteAccess()

                text: qsTr("Change connection settings")

                clickedFunc: function () {
                    changeSettingsDrawer.openTriggered()
                }
            }

            LabelWithButtonType {
                id: removeButton

                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 24
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                visible: ServersModel.isProcessedServerHasWriteAccess()

                text: qsTr("Remove ") + ContainersModel.getProcessedContainerName()
                textColor: AmneziaStyle.color.vibrantRed

                clickedFunction: function () {
                    var headerText = qsTr("Remove %1 from server?").arg(ContainersModel.getProcessedContainerName())
                    var descriptionText = qsTr("The proxy will be stopped and all users will lose access.")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function () {
                        PageController.goToPage(PageEnum.PageDeinstalling)
                        InstallController.removeProcessedContainer()
                    }
                    var noButtonFunction = function () {
                    }

                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }

                MouseArea {
                    anchors.fill: removeButton
                    cursorShape: Qt.PointingHandCursor
                    enabled: false
                }
            }

            DividerType {
                visible: ServersModel.isProcessedServerHasWriteAccess()
            }
        }
    }
}
