import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"


PageType {
    id: root

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

        model: WireGuardConfigModel

        delegate: ColumnLayout {
            width: listView.width

            property alias mtuTextField: mtuTextField
            property bool isSaveButtonEnabled: mtuTextField.errorText === ""

            spacing: 0

            BaseHeaderType {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("WG settings")
            }

            TextFieldWithHeaderType {
                id: mtuTextField
                Layout.fillWidth: true
                Layout.topMargin: 40
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("MTU")
                textField.text: clientMtu
                textField.validator: IntValidator { bottom: 576; top: 65535 }

                textField.onEditingFinished: {
                    if (textField.text !== clientMtu) {
                        clientMtu = textField.text
                    } 
                }
                checkEmptyText: true
            }

            Header2TextType {
                Layout.fillWidth: true
                Layout.topMargin: 16
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                text: qsTr("Server settings")
            }

            TextFieldWithHeaderType {
                id: portTextField
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                enabled: false

                headerText: qsTr("Port")
                textField.text: port
            }
        }

        footer: ColumnLayout {
            width: listView.width

            BasicButtonType {
                id: saveButton

                Layout.fillWidth: true
                Layout.topMargin: 24
                Layout.bottomMargin: 24
                Layout.rightMargin: 16
                Layout.leftMargin: 16

                enabled: listView.currentItem.isSaveButtonEnabled

                text: qsTr("Save")

                clickedFunc: function() {
                    var headerText = qsTr("Save settings?")
                    var descriptionText = qsTr("Only the settings for this device will be changed")
                    var yesButtonText = qsTr("Continue")
                    var noButtonText = qsTr("Cancel")

                    var yesButtonFunction = function() {
                        if (ConnectionController.isConnected && ServersModel.getDefaultServerData("defaultContainer") === ContainersModel.getProcessedContainerIndex()) {
                            PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                            return
                        }

                        PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                        InstallController.updateContainer(WireGuardConfigModel.getConfig())
                    }
                    var noButtonFunction = function() {}
                    showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
                }
            }
        }
    }
}
