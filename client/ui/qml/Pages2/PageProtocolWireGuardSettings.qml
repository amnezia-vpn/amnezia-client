import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import SortFilterProxyModel 0.2

import PageEnum 1.0
import ProtocolEnum 1.0
import Style 1.0

import "./"
import "../Controls2"
import "../Controls2/TextTypes"
import "../Config"
import "../Components"

PageType {
    id: root

    ListViewType {
        id: listView

        anchors.top: parent.top
        anchors.bottom: saveButton.top
        anchors.bottomMargin: 16
        anchors.right: parent.right
        anchors.left: parent.left

        enabled: ServersUiController.isProcessedServerHasWriteAccess()

        header: NavigationBarType {
            width: listView.width
            height: implicitHeight + 16

            title: qsTr("WG settings")
        }

        model: WireGuardConfigModel

        delegate: ColumnLayout {
            width: listView.width

            property bool isSaveButtonEnabled: portTextField.errorText === "" &&
                                               vpnAddressSubnetTextField.errorText === ""

            spacing: 12

            TextFieldType {
                id: vpnAddressSubnetTextField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("VPN address subnet")
                textField.text: subnetAddress

                textField.onEditingFinished: {
                    if (textField.text !== subnetAddress) {
                        subnetAddress = textField.text
                    }
                }

                checkEmptyText: true
            }

            TextFieldType {
                id: portTextField

                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16

                headerText: qsTr("Port")
                textField.text: port
                textField.maximumLength: 5
                textField.validator: IntValidator { bottom: 1; top: 65535 }

                textField.onEditingFinished: {
                    if (textField.text !== port) {
                        port = textField.text
                    }
                }

                checkEmptyText: true
            }
        }
    }

    ButtonType {
        id: saveButton

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        anchors.bottomMargin: 8 + PageController.safeAreaBottomMargin

        enabled: listView.enabled && listView.currentItem !== null && listView.currentItem.isSaveButtonEnabled

        text: qsTr("Save")

        onClicked: function() {
            forceActiveFocus()

            var headerText = qsTr("Save settings?")
            var descriptionText = qsTr("All users with whom you shared a connection with will no longer be able to connect to it.")
            var yesButtonText = qsTr("Continue")
            var noButtonText = qsTr("Cancel")

            var yesButtonFunction = function() {
                if (ConnectionController.isConnected && ServersUiController.serverDefaultContainer(ServersUiController.defaultServerId) === ServersUiController.processedContainerIndex) {
                    PageController.showNotificationMessage(qsTr("Unable change settings while there is an active connection"))
                    return
                }

                PageController.goToPage(PageEnum.PageSetupWizardInstalling);
                InstallController.updateServerConfig(ServersUiController.processedServerId, ServersUiController.processedContainerIndex, ProtocolEnum.WireGuard)
            }
            var noButtonFunction = function() {}
            showQuestionDrawer(headerText, descriptionText, yesButtonText, noButtonText, yesButtonFunction, noButtonFunction)
        }
    }
}
